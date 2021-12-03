/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
*/

/** @file
 *
 * This minimal "filesystem" provides XSearch capabilities
 *
 * Compile with:
 *
 *     g++ -Wall xsfs.cpp `pkg-config fuse3 --cflags --libs` -o xsfs
 * 
 * Additional compiler flags for warnings:
 * 		-Wwrite-strings -Wformat-extra-args -Wformat=
 */

#define FUSE_USE_VERSION 31

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 700
#endif

extern "C" {
	#include <fuse.h>
	#include <fuse_lowlevel.h>

	#include "fs_helpers.h"
}

#include <iostream>
// #include <cmath>
#include <cerrno>
#include <cstring>
#include <clocale>
#include <dirent.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <thread>
#include <utility>
#include <atomic>
#include <memory>

#include "ouroboros.hpp"
#include "xs_helpers.hpp"

using namespace std;
using namespace ouroboros;

#define BUFFER_SIZE 1024
#define PORT 8080
#define MAX_RESULTS 2

/* XSearch variables declaration */

int id;
int queue_size;
int block_size;
long page_size;
// the manager is used to store and provide access to the queues
TFIDFIndexMemoryComponentType store_type;
MemoryComponentManager* manager;
FileDualQueueMemoryComponent* component;
DualQueue<FileDataBlock*> *queue;
// atomic int storing the total number of blocks read
atomic<long> total_num_tokens(0);

/* Server for XSearch queries */

void server() {
    int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	
	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	
	// Forcefully attaching socket to the port 8080
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
												&opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );
	
	// Forcefully attaching socket to the port 8080
	if (bind(server_fd, (struct sockaddr *)&address,
								sizeof(address)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

    while ( 1 )
	{
		if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
						(socklen_t*)&addrlen)) < 0)
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}

		// create empty buffer every time
		char search_term[BUFFER_SIZE] = {0};
		valread = read(new_socket, search_term, BUFFER_SIZE);    
		cout << "[server] " << search_term << endl; 

		// process query
        int frequency = 0;
        long numFiles = MAX_RESULTS;
        
        cout << "- " << search_term << " : [ " << flush;
        
        TFIDFIndexMemoryComponent* componentIndex;
		FileIndexMemoryComponent* componentFileIndex;
		shared_ptr<BaseTFIDFIndex> index;
		shared_ptr<BaseFileIndex> fileIndex;
		shared_ptr<TFIndexResult> result;
		long idx;
		
		// get the TFIDF index component identified by index_id
		componentIndex = (TFIDFIndexMemoryComponent*) 
							manager->getMemoryComponent(MemoryComponentType::TFIDF_INDEX, id);

		// get the TFIDF index from the component
		index = componentIndex->getTFIDFIndex();

		// get the file index component identified by index_id modulo num_readers
		componentFileIndex = (FileIndexMemoryComponent*)
								manager->getMemoryComponent(MemoryComponentType::FILE_INDEX, id);

		// get the file index from the component
		fileIndex = componentFileIndex->getFileIndex();

		// search the index for the query term and return the term and inverse document frequencies
		result = index->lookup(search_term);
		for (int j = 0; (j < result->files.size()) and (numFiles > 0); j++, numFiles--) {
			const char* file_path = fileIndex->reverseLookup(result->files[j].fileIdx);
			cout << file_path << ":" << result->files[j].fileFrequency << " " << flush;	
		}
		frequency += result->termFrequency;
        
        if (numFiles == 0) {
            cout << "... ] " << frequency << endl;
        } else {
            cout << "] " << frequency << endl;
        }

		// notify the client
		char *msg = "Success!";
		send(new_socket, msg, strlen(msg), 0);
    }
}

/* FUSE operations */

/* Get file attributes. Similar to stat() */
static int xs_getattr(const char *path, struct stat *stbuf,
			struct fuse_file_info *fi)
{
	cout << "[getattr] from " << path << endl;

	(void) fi;
	int res;

	res = lstat(path, stbuf);
	if (res == -1)
		return - errno;

	return 0;
}

static int xs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	cout << "[mknod] at " << path << endl;

	int res;

	res = mknod_wrapper(AT_FDCWD, path, NULL, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xs_mkdir(const char *path, mode_t mode)
{
	cout << "[mkdir] at " << path << endl;

	int res;

	res = mkdir(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

/* Remove a file. Not related to symbolic/hard links */
static int xs_unlink(const char *path)
{
	cout << "[unlink] at " << path << endl;

	int res;

	res = unlink(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xs_rmdir(const char *path)
{
	cout << "[rmdir] at " << path << endl;

	int res;

	res = rmdir(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xs_rename(const char *from, const char *to, unsigned int flags)
{
	cout << "[rename] " << from << " to " << to << endl;

	int res;

	if (flags)
		return -EINVAL;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xs_open(const char *path, struct fuse_file_info *fi)
{
	cout << "[open] at " << path << endl;

	int res;

	res = open(path, fi->flags);
	if (res == -1)
		return -errno;

	fi->fh = res;
	return 0;
}

static int xs_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	cout << "[read] " << size << " bytes from " << path << endl;

	int fd;
	int res;

	if(fi == NULL)
		fd = open(path, O_RDONLY);
	else
		fd = fi->fh;
	
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);
	return res;
}

static int xs_write(const char *path, const char *buf, size_t size,
		    off_t offset, struct fuse_file_info *fi)
{
	cout << "[write] " << size << " bytes in " << path << endl;

	int fd;
	int res;

	(void) fi;
	if(fi == NULL)
		fd = open(path, O_WRONLY);
	else
		fd = fi->fh;
	
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);
	return res;
}

static int xs_statfs(const char *path, struct statvfs *stbuf)
{
	cout << "[statfs] call" << endl;

	int res;

	res = statvfs(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

/* Release an open file when there are no more references to it */
static int xs_release(const char *path, struct fuse_file_info *fi)
{
	cout << "[release] at " << path << endl;

	work_read(manager,  const_cast<char*>(path), id, block_size);
	work_index(manager, &total_num_tokens, id, id, block_size + BLOCK_ADDON_SIZE);

	cout << "tokens: " << total_num_tokens << endl;
	
	(void) path;
	close(fi->fh);

	return 0;
}

static int xs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi,
		       enum fuse_readdir_flags flags)
{
	cout << "[readdir] at " << path << endl;

	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;
	(void) flags;

	dp = opendir(path);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0, FUSE_FILL_DIR_PLUS))
                        break;
	}

	closedir(dp);
	return 0;
}

static void *xs_init(struct fuse_conn_info *conn,
		      struct fuse_config *cfg)
{
	cout << "[init] filesystem" << endl;

	(void) conn;
	cfg->use_ino = 1;

	/* Pick up changes from lower filesystem right away.
	   Also necessary for better hardlink support */
	cfg->entry_timeout = 0;
	cfg->attr_timeout = 0;
	cfg->negative_timeout = 0;

	return NULL;
}

/* Check file access permissions */
static int xs_access(const char *path, int mask)
{
	cout << "[access] to path " << path << endl;

	int res;

	res = access(path, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xs_create(const char *path, mode_t mode,
		    struct fuse_file_info *fi)
{
	cout << "[create] at " << path << endl;

	int res;

	res = open(path, fi->flags, mode);
	if (res == -1)
		return -errno;

	fi->fh = res;
	return 0;
}

/* Change the access and modification times of a file with ns resolution */
#ifdef HAVE_UTIMENSAT
static int xs_utimens(const char *path, const struct timespec ts[2],
		       struct fuse_file_info *fi)
{
	cout << "[utimens] on " << path << endl;

	(void) fi;
	int res;

	/* don't use utime/utimes since they follow symlinks */
	res = utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
	if (res == -1)
		return -errno;

	return 0;
}
#endif

/* Allocates space for an open file */
#ifdef HAVE_POSIX_FALLOCATE
static int xs_fallocate(const char *path, int mode,
			off_t offset, off_t length, struct fuse_file_info *fi)
{
	cout << "[fallocate] file " << path << endl;
	
	int fd;
	int res;

	(void) fi;

	if (mode)
		return -EOPNOTSUPP;

	if(fi == NULL)
		fd = open(path, O_WRONLY);
	else
		fd = fi->fh;
	
	if (fd == -1)
		return -errno;

	res = -posix_fallocate(fd, offset, length);

	if(fi == NULL)
		close(fd);
	return res;
}
#endif

/* Find next data or hole after the specified offset */
static off_t xs_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi)
{
	cout << "[lseek] " << whence << " at " << path << endl;

	int fd;
	off_t res;

	if (fi == NULL)
		fd = open(path, O_RDONLY);
	else
		fd = fi->fh;

	if (fd == -1)
		return -errno;

	res = lseek(fd, off, whence);
	if (res == -1)
		res = -errno;

	if (fi == NULL)
		close(fd);
	return res;
}

/* Note: out-of-order designated initialization is supported 
   in the C programming language, but is not allowed in C++. */
static const struct fuse_operations xs_oper = {
	.getattr	= xs_getattr,
	.mknod		= xs_mknod,
	.mkdir		= xs_mkdir,
	.unlink		= xs_unlink,
	.rmdir		= xs_rmdir,
	.rename		= xs_rename,
	.open		= xs_open,
	.read		= xs_read,
	.write		= xs_write,
	.statfs		= xs_statfs,
	.release	= xs_release,
	.readdir	= xs_readdir,
	.init       = xs_init,
	.access		= xs_access,
	.create 	= xs_create,
#ifdef HAVE_UTIMENSAT
	.utimens	= xs_utimens,
#endif
#ifdef HAVE_POSIX_FALLOCATE
	.fallocate	= xs_fallocate,
#endif
	.lseek		= xs_lseek,
};

int main(int argc, char *argv[])
{
	id = 1;
	queue_size = QUEUE_SIZE_RATIO;
	block_size = BLOCK_SIZE;
	page_size = PAGE_SIZE;
	store_type = TFIDFIndexMemoryComponentType::PSTD;
	// create each queue add the queues to the manager
    manager = new MemoryComponentManager();

	work_init_queue(manager, id, queue_size, block_size);
	work_init_index(manager, id, page_size, INIT_CAPACITY, store_type);

    // launch server on separate thread 
    thread fooThread(server); 

	umask(0);
	// run the filesystem
	return fuse_main(argc, argv, &xs_oper, NULL);

	// free all memory components
    // delete manager;
}