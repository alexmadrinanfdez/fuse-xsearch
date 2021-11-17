/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPLv2.
  See the file COPYING.
*/

/** @file
 *
 * This "filesystem" provides xsearch capabilities
 *
 * Compile with:
 *
 *     g++ -Wall xsearch-fuse.cpp `pkg-config fuse3 --cflags --libs` -o xsfs
 *
 * ## Source code ##
 * \include xsearch.c
 */


#define FUSE_USE_VERSION 31

extern "C" {
	#include <fuse.h>
	#include <fuse_lowlevel.h>
}
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <time.h>
// #include <errno.h>

#include <iostream>
// #include <cmath>
#include <cerrno>
#include <cstring>
#include <clocale>

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <thread>

#define SERVER_BUFFER_SIZE 1024
#define SERVER_PORT 8080

/* Part to implement FUSE functions.
 * 
 */


static int xsfs_getattr(const char *path, struct stat *stbuf,
			struct fuse_file_info *fi)
{
	(void) fi;
	int res;

	res = lstat(path, stbuf);
	if (res == -1)
		return - errno;

	return 0;
}


/* Part to implement the server for xsearch
 * queries 
 */

void server() {

    int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[SERVER_BUFFER_SIZE] = {0};
	
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
	address.sin_port = htons( SERVER_PORT );
	
	// Forcefully attaching socket to the port 8080
	if (bind(server_fd, (struct sockaddr *)&address,
								sizeof(address))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

    while (1==1) {

	if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
					(socklen_t*)&addrlen))<0)
	{
		perror("accept");
		exit(EXIT_FAILURE);
	}  

    valread = read( new_socket , buffer, SERVER_BUFFER_SIZE);    
    std::cout << "[server] " << buffer << std::endl;


    // TODO : process query 

    char *hello = "Query received"; 
	send(new_socket , hello , strlen(hello) , 0 );

    }

}


static const struct fuse_operations xsfs_oper = {
	.getattr	= xsfs_getattr,
};

int main(int argc, char *argv[])
{

    // Launch server on separate thread 
    std::thread fooThread(server); 

	umask(0);
	return fuse_main(argc, argv, &xsfs_oper, NULL);
}
