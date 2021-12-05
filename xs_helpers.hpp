using namespace std;
using namespace ouroboros;

#define BLOCK_SIZE 1024
#define QUEUE_SIZE 1024
#define DELIMITERS " \t\n"
#define INIT_CAPACITY 1 << 16 // == pow(2, 16) (bit shifting)

/* XSearch functions */

void work_idx(BaseTermIndex *termIndex, BaseTermFileRelation *invertedIndex, CTokBLock *block, long fileIdx)
{
    long termIdx;
    long invertedIdx;

    for (auto i = 0; i < block->numTokens; ++i) {
        termIdx = termIndex->insert(block->tokens[i]);
        invertedIdx = invertedIndex->insert(termIdx, fileIdx);
    }
}

void work_tokidx(DualQueue<FileDataBlock*> *queue, 
                 BaseTermIndex *termIndex,
                 BaseTermFileRelation *invertedIndex,
				 atomic<long>* total_num_tokens)
{
    FileDataBlock *dataBlock;
    BranchLessTokenizer *tokenizer;
    CTokBLock *tokBlock;
    char *buffer;
    char **tokens;
    char delims[32] = DELIMITERS;
    int length;

    buffer = new char[BLOCK_SIZE + 1];
    tokens = new char*[BLOCK_SIZE / 2 + 1];
    tokBlock = new CTokBLock();
    tokBlock->buffer = buffer;
    tokBlock->tokens = tokens;
    tokenizer = new BranchLessTokenizer(delims);

    while (true) {
        dataBlock = queue->pop_full();
        
        length = dataBlock->length;
        if (length > 0) {
            tokenizer->getTokens(dataBlock, tokBlock);
            work_idx(termIndex, invertedIndex, tokBlock, dataBlock->fileIdx);
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

	*total_num_tokens += termIndex->getNumTerms();
}

void work_read(DualQueue<FileDataBlock*> *queue,
               BaseFileIndex *fileIndex,
               char *filename)
{
    PFileReaderDriver *reader;
    FileDataBlock *dataBlock;
	FileDataBlock *finalBlock;
    int pos;
    long fileIdx;

    reader = new PFileReaderDriver(filename, BLOCK_SIZE);
	try {
		reader->open();
	} catch (exception &e) {
		cout << "ERR: could not open file " << filename << endl;
		delete reader;
		return;    
	}

	fileIdx = fileIndex->lookup(filename);
	if (fileIdx == INDEX_NEXISTS) {
		fileIndex->insert(filename);
		cout << "INFO: no previous index for file " << filename << endl;
		fileIdx = fileIndex->lookup(filename);
	}

	while (true) {
		dataBlock = queue->pop_empty();

		reader->readNextBlock(dataBlock);
		dataBlock->fileIdx = fileIdx;

		queue->push_full(dataBlock);

		if (dataBlock->length == 0) {
			// signal end of the tokens
			finalBlock = queue->pop_empty();
			finalBlock->length = -1;
			queue->push_full(finalBlock);
			break;
		}
	}

	reader->close();
	delete reader;
}