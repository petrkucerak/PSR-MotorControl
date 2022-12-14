/*
 * HTTP.c
 *
 *  Created on: Dec 14, 2022
 *      Author: kucerp28
 */

#include "HTTP.h"

#include <netdb.h>
#include <netinet/in.h>
#include <sockLib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define SERVER_PORT 80 /* Port 80 is reserved for HTTP protocol */
#define SERVER_MAX_CONNECTIONS 20
#define WEB_RESPONSE_PRIORITY 255

void www() {
	int s;
	int newFd;
	struct sockaddr_in serverAddr;
	struct sockaddr_in clientAddr;
	int sockAddrSize;

	sockAddrSize = sizeof(struct sockaddr_in);
	bzero((char*) &serverAddr, sizeof(struct sockaddr_in));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		fprintf(stderr, "Error: www: socket(%d)\n", s);
		return;
	}

	if (bind(s, (struct sockaddr*) &serverAddr, sockAddrSize) == ERROR) {
		fprintf(stderr, "Error: www: bind\n");
		return;
	}

	if (listen(s, SERVER_MAX_CONNECTIONS) == ERROR) {
		perror("www listen");
		close(s);
		return;
	}

	printf("www server running\n");

	while (1) {
		/* accept waits for somebody to connect and the returns a new file
		 * descriptor */
		if ((newFd = accept(s, (struct sockaddr*) &clientAddr, &sockAddrSize))
				==
				ERROR) {
			perror("www accept");
			close(s);
			return;
		}

		/* The client connected from IP address inet_ntoa(clientAddr.sin_addr)
		 and port ntohs(clientAddr.sin_port).

		 Start a new task for each request. The task will parse the request
		 and sends back the response.

		 Don't forget to close newFd at the end */
		// Spawn tasks
		char taskName[60];
		sprintf(taskName, "tHttpRes%lu", (unsigned) time(NULL));
		TASK_ID tHttpRes = taskSpawn(taskName, WEB_RESPONSE_PRIORITY, 0, 4096,
				(FUNCPTR) serverResponse, newFd, 0, 0, 0, 0, 0, 0, 0, 0, 0);

		if (tHttpRes == NULL) {
			fprintf(stderr, "ERROR: Can't spawn a web server response task\n");
		}

	}
}

void serverResponse(int newFd) {
	FILE *f = fdopen(newFd, "w");
	/* Print the HTTP Response Header */
	fprintf(f, "HTTP/1.0 200 OK\r\n");
	fprintf(f, "Content-Type: text/html\r\n");

	fprintf(f, "\r\n");

	/* Print the HTTP Response Body */
	fprintf(f,
			"<!DOCTYPE html PUBLIC \"-//IETF//DTD HTML 2.0//EN\"> <html> <head> <title> Hello PSR!</title> </head> <body> <h1>Hi</h1> <p>This is very minimal \"hello world\" HTML document.</p><p>And current time is: %ld</p> </body> </html>\r\n",
			time(NULL));
	fclose(f);
	close(newFd);

}
