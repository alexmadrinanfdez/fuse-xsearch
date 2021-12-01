using namespace std;
using namespace ouroboros;

#define QUEUE_SIZE_RATIO 2
#define BLOCK_ADDON_SIZE 4096
#define BLOCK_SIZE 1024
#define PAGE_SIZE 4096
#define DELIMITERS " \t\n"
#define INIT_CAPACITY 1 << 16 // == pow(2, x) (bit shifting)

/* XSearch functions */

void work_index(MemoryComponentManager* manager,
                atomic<long>* total_num_tokens,
                unsigned int queue_id,
                unsigned int index_id,
                int block_size)
{
	TFIDFIndexMemoryComponent* componentIndex;
	shared_ptr<BaseTFIDFIndex> index;
	FileDualQueueMemoryComponent* componentQueue;
	DualQueue<FileDataBlock*> *queue;
	FileDataBlock *dataBlock;
	BranchLessTokenizer *tokenizer;
	CTokBLock *tokBlock;
	char *buffer;
	char **tokens;
	char delims[32] = DELIMITERS;
	int length;

	// allocate the buffers, the list of tokens for the tokenizer data block and create the 
	buffer = new char[block_size + 1];
	tokens = new char*[block_size / 2 + 1];
	tokBlock = new CTokBLock();
	tokBlock->buffer = buffer;
	tokBlock->tokens = tokens;
	tokenizer = new BranchLessTokenizer(delims);

	// get the paged string store component identified by worker_id from the manager
	componentIndex = (TFIDFIndexMemoryComponent*) 
						manager->getMemoryComponent(MemoryComponentType::TFIDF_INDEX, index_id);
	// get the store from the store component
	index = componentIndex->getTFIDFIndex();
	// get the queue component identified by numa_id from the manager
	componentQueue = (FileDualQueueMemoryComponent*)
						manager->getMemoryComponent(MemoryComponentType::DUALQUEUE, queue_id);
	// get the queue from the queue component
	queue = componentQueue->getDualQueue();

	// load balancing is achieved through the queue
	while (true) {
		// pop full data block from the queue
		dataBlock = queue->pop_full();
		// if the data in the block has a length greater than 0 then tokenize, otherwise exit the while loop
		length = dataBlock->length;
		if (length > 0) {
			tokenizer->getTokens(dataBlock, tokBlock);

			for (long i = 0; i < tokBlock->numTokens; i++) {
				index->insert(tokBlock->tokens[i], dataBlock->fileIdx);
			}
		}
		
		queue->push_empty(dataBlock);

		if (length == -1) {
			break;
		}
	}

	delete tokenizer;
	delete tokBlock;
	delete[] tokens;
	delete[] buffer;

	*total_num_tokens += index->getNumTerms();
}

void work_read(MemoryComponentManager* manager,
               char *filename,
               unsigned int queue_id,
               int block_size)
{
	FileDualQueueMemoryComponent* componentQueue;
	FileIndexMemoryComponent* componentFileIndex;
	DualQueue<FileDataBlock*> *queue;
	shared_ptr<BaseFileIndex> index;
	WaveFileReaderDriver *reader;
	FileDataBlock *dataBlock;
	FileDataBlock *finalBlock;
	char delims[32] = DELIMITERS;
	int i, length;
	long fileIdx;

	// get the queue component identified by queue_id
	componentQueue = (FileDualQueueMemoryComponent*)
						manager->getMemoryComponent(MemoryComponentType::DUALQUEUE, queue_id);
	// get the queue from the queue component
	queue = componentQueue->getDualQueue();
	// get the file index component identified by queue_id
	componentFileIndex = (FileIndexMemoryComponent*)
							manager->getMemoryComponent(MemoryComponentType::FILE_INDEX, queue_id);
	// get the file index from the file index component
	index = componentFileIndex->getFileIndex();
	// create a new reader driver for the current file
	reader = new WaveFileReaderDriver(filename, block_size, BLOCK_ADDON_SIZE, delims);

	// try to open the file in the reader driver; if it errors out print message and terminate
	try {
		reader->open();
		fileIdx = index->insert(filename);
	} catch (exception &e) {
		cout << "ERR: could not open file " << filename << endl;
		delete reader;
		return;
	}

	while (true) {
		// pop empty data block from the queue
		dataBlock = queue->pop_empty();
		// read a block of data from the file into data block buffer
		reader->readNextBlock(dataBlock);
		dataBlock->fileIdx = fileIdx;
		length = dataBlock->length;
		// push full data block to queue (in this case it pushed to the empty queue since there is no consumer)
		queue->push_full(dataBlock);
		// if the reader driver reached the end of the file break from the while loop and read next file
		if (length == 0) {
			// signal end of the tokens
			finalBlock = queue->pop_empty();
			finalBlock->length = -1;
			queue->push_full(finalBlock);

			break;
			}
		}

		// close the reader and free memory
		reader->close();
		delete reader;
}

void work_init_indexes(MemoryComponentManager* manager,
                       unsigned int index_id,
                       long page_size,
                       long initial_capacity,
                       TFIDFIndexMemoryComponentType store_type)
{
    TFIDFIndexMemoryComponent* component;
	unsigned long numBuckets;
	size_t bucketSize = ChainedHashTable<const char*,
						PagedVersatileIndex<TFIndexEntry, IDFIndexEntry>*,
						cstr_hash,
						cstr_equal>::getBucketSizeInBytes();
	numBuckets = get_next_prime_number(initial_capacity / bucketSize);
	// create a new page string store component; the component is responsible with storing the terms sequentially
	component = new TFIDFIndexMemoryComponent(page_size, store_type, numBuckets);
	// add the string store component to the manager identified by the current worker_id
	manager->addMemoryComponent(MemoryComponentType::TFIDF_INDEX, index_id, component);
}

void work_init_queues(MemoryComponentManager* manager,
                      unsigned int queue_id,
                      int queue_size,
                      int block_size)
{
	FileDualQueueMemoryComponent* componentQueue;
	FileIndexMemoryComponent* componentFileIndex;

	// create a new queue component; the component is responsible with intializing the queue and the queue elements
	componentQueue = new FileDualQueueMemoryComponent(queue_size, block_size + BLOCK_ADDON_SIZE);
	// create a new file index component; the component is responsible for maintaining the indexes of file paths
	componentFileIndex = new FileIndexMemoryComponent(FileIndexMemoryComponentType::STD);
	// add the queue component to the manager, identified by the current queue_id
	manager->addMemoryComponent(MemoryComponentType::DUALQUEUE, queue_id, componentQueue);
	// add the file index component to the manager, identified by the current queue_id
	manager->addMemoryComponent(MemoryComponentType::FILE_INDEX, queue_id, componentFileIndex);
}
