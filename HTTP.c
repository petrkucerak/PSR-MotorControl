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

#define ID http_d->cycle_p

HTTP_D* initHTTPData() {

	/* Allocated the main strucutre */
	HTTP_D *http_d = (HTTP_D*) malloc(sizeof(HTTP_D));
	if (http_d == NULL) {
		fprintf(stderr,
				"ERROR: Can't allocate memory for the HTTP_D structure! [HTTP.c]\n");
		exit(1);
	}

	/* Init a http data mutex */
	http_d->http_sem = semMCreate(SEM_Q_FIFO);

	/* set cycle pointer*/
	http_d->cycle_p = 0;

	return http_d;
}

void saveHTTPData(HTTP_D *http_d, int motor_position, int requested_position,
		int pwm_speed) {

	ID++;
	if (ID == CYCLE_SIZE)
		ID = 0;
	semTake(http_d->http_sem, WAIT_FOREVER);
	http_d->motor_position[ID] = motor_position;
	http_d->requested_position[ID] = requested_position;
	http_d->pwm_speed[ID] = pwm_speed;
	semGive(http_d->http_sem);
}

void handleHTTPData(UDP *udp, struct psrMotor *my_motor, HTTP_D *http_d,
		int *isEndp) {

	while (!*isEndp) {
		int motor_position = getMotorSteps();
		int requested_position = udp->wanted_position;
		int pwm_speed = getPWM(my_motor);
		saveHTTPData(http_d, motor_position, requested_position, pwm_speed);
		printf(
				"Motor position: %d\nRequested position: %d\nPWM speed: %d\n\n",
				motor_position, requested_position, pwm_speed);
		taskDelay(100);
	}
}

void removeHTTPData(HTTP_D *http_d) {
	free(http_d);
	http_d = NULL;
}
void www(int *isEndp) {
	int s;
	int newFd;
	struct sockaddr_in serverAddr;
	struct sockaddr_in clientAddr;
	socklen_t sockAddrSize;

	sockAddrSize = sizeof(struct sockaddr_in);
	bzero((char*) &serverAddr, sizeof(struct sockaddr_in));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		fprintf(stderr, "ERROR: www: socket(%d)! [HTTP.c]\n", s);
		return;
	}

	if (bind(s, (struct sockaddr*) &serverAddr, sockAddrSize) == ERROR) {
		fprintf(stderr, "ERROR: www: bind! [HTTP.c]\n");
		return;
	}

	if (listen(s, SERVER_MAX_CONNECTIONS) == ERROR) {
		perror("www listen");
		close(s);
		return;
	}

	printf("www server running [HTTP.c]\n");

	while (!*isEndp) {
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

		 Start a new task for each reqHTTPuest. The task will parse the request
		 and sends back the response.

		 Don't forget to close newFd at the end */
		// Spawn tasks
		char taskName[60];
		sprintf(taskName, "tHttpRes%lu", (unsigned) time(NULL));
		TASK_ID tHttpRes = taskSpawn(taskName, WEB_RESPONSE_PRIORITY, 0, 4096,
				(FUNCPTR) serverResponse, newFd, 0, 0, 0, 0, 0, 0, 0, 0, 0);

		if (tHttpRes == NULL) {
			fprintf(stderr,
					"ERROR: Can't spawn a web server response task [HTTP.c]\n");
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
			"<!DOCTYPE html><html lang=\"cs\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><script src=\"https://cdn.tailwindcss.com\"></script><title>Motor Control - Dashboard</title></head><body onload=\"setTimeout(function(){location.reload()}, 100);\" class=\"flex justify-center w-full\"><h1 class=\"text-4xl font-bold\">Dashboard a motor control</h1><p>%d</p></body></html>\r\n",
			time(NULL));
	fclose(f);
	close(newFd);

}
