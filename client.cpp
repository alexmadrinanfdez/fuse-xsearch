/** @file
 *
 * Client side C/C++ program to demonstrate socket programming
 *
 * Compile with:
 *
 *     g++ -Wall -Wextra client.cpp -o client
 */

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

#define PORT 8080
#define BUFFER_SIZE 1024

int main(int argc, char const *argv[])
{

	if (argc < 2) {
		
		std::cout << "usage: " << argv[0] << " <query>" << std::endl;
		return 1;
	}

	int sock = 0, valread;
	struct sockaddr_in serv_addr;
	// char *hello = "[client] hello";
	char buffer[BUFFER_SIZE] = {0};
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Socket creation error \n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	
	// Convert IPv4 and IPv6 addresses from text to binary form
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
	{
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\nConnection Failed \n");
		return -1;
	}

	send(sock, argv[1], strlen(argv[1]), 0 );
	// printf("Hello message sent\n");
	valread = read(sock, buffer, BUFFER_SIZE);
	printf("%s\n", buffer);
	return 0;
}
