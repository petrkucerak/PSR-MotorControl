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
#include <string.h>

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
	for (int i = 0; i < CYCLE_SIZE; ++i) {
		http_d->motor_position[i] = 0;
		http_d->requested_position[i] = 0;
		http_d->pwm_speed[i] = 0;
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
		//printf("Motor position: %d\nRequested position: %d\nPWM speed: %d\n\n",
		//		motor_position, requested_position, pwm_speed);
		taskDelay(1);
	}
}

void removeHTTPData(HTTP_D *http_d) {
	free(http_d);
	http_d = NULL;
}
void www(int *isEndp, HTTP_D *http_d) {
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
				(FUNCPTR) serverResponse, newFd, (HTTP_D*) http_d, 0, 0, 0, 0,
				0, 0, 0, 0);

		if (tHttpRes == NULL) {
			fprintf(stderr,
					"ERROR: Can't spawn a web server response task [HTTP.c]\n");
		}

	}
}

void serverResponse(int newFd, HTTP_D *http_d) {

	/* Parse the HTTP request */
	FILE *f = fdopen(newFd, "r+");
	setvbuf(f, NULL, _IONBF, 0); /* Disable buffering to work around VxWorks bug */
	char filename[100], http_ver[10];
	int ret = fscanf(f, "GET %99s %9s\n", filename, http_ver);
	if (ret != 2) {
		fprintf(stderr, "www: Request reading error\n");
		// ...
	}
	printf("GET %s %s\n", filename, http_ver);

	setvbuf(f, NULL, _IOFBF, BUFSIZ); /* Reenable buffering */

	if (strcmp(filename, "/motor.svg") == 0 || strcmp(filename, "/motor1.svg") == 0) {
		// printf("motor.svg\n");
		/* Motor position */
		/* Find a max and min value to create q graph scale */
		int max_position = INT_MIN;
		int min_position = INT_MAX;
		for (int i = 0; i < CYCLE_SIZE; ++i) {
			/* find a max value*/
			if (http_d->motor_position[i] > max_position)
				max_position = http_d->motor_position[i];
			if (http_d->requested_position[i] > max_position)
				max_position = http_d->requested_position[i];
			/* find a min value*/
			if (http_d->motor_position[i] < min_position)
				min_position = http_d->motor_position[i];
			if (http_d->requested_position[i] < min_position)
				min_position = http_d->requested_position[i];
		}

		int difference = max_position - min_position;
		if (difference < 10) {
			max_position += 10;
			min_position -= 10;
		}

		/* Add 20% to scale on both sides*/
		max_position += (int) (difference * 0.2);
		min_position -= (int) (difference * 0.2);

		difference = max_position - min_position;
		int step = (int) (difference * 0.25);

		int y_scale_0 = min_position;
		int y_scale_1 = min_position + (step * 1);
		int y_scale_2 = min_position + (step * 2);
		int y_scale_3 = min_position + (step * 3);
		int y_scale_4 = max_position;

		/* Print the HTTP Response Header */
		fprintf(f, "HTTP/1.0 200 OK\r\n");
		fprintf(f, "Content-Type: image/svg+xml\r\n");

		fprintf(f, "\r\n");

		fprintf(f,
				"<svg version=\"1.2\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" class=\"graph\" aria-labelledby=\"title\" role=\"img\"> <title id=\"title\">Actual and Requested Motor Position</title> <g class=\"grid x-grid\" id=\"xGrid\"> <line x1=\"90\" x2=\"90\" y1=\"5\" y2=\"371\"></line> </g> <g class=\"grid y-grid\" id=\"yGrid\"> <line x1=\"90\" x2=\"705\" y1=\"370\" y2=\"370\"></line> </g> <g class=\"labels x-labels\"><text x=\"100\" y=\"392\">-2000</text> <text x=\"250\" y=\"392\">-1500</text> <text x=\"400\" y=\"392\">-1000</text> <text x=\"550\" y=\"392\">-500</text> <text x=\"700\" y=\"392\">0</text> <text x=\"400\" y=\"430\" class=\"label-title\">time [ms]</text> </g> <g class=\"labels y-labels\">");

		fprintf(f,
				"<text x=\"80\" y=\"15\">%d</text> <text x=\"80\" y=\"95\">%d</text> <text x=\"80\" y=\"189\">%d</text> <text x=\"80\" y=\"273\">%d</text> <text x=\"80\" y=\"380\">%d</text><text x=\"-115\" y=\"40\" class=\"label-title\" transform=\"rotate(-90)\">motor position [step]</text> </g>",
				y_scale_4, y_scale_3, y_scale_2, y_scale_1, y_scale_0);

		fprintf(f,
				"<polyline fill=\"none\" stroke=\"#0074d9\" stroke-width=\"2\" points=\"\n");
		// x step is 0.61
		for (int i = 0; i < CYCLE_SIZE; ++i) {
			int point = ID - i;
			if (point < 0)
				point += CYCLE_SIZE;
			fprintf(f, "%f, %f\n", 700 - (i * 0.61),
					(370
							- (float) ((http_d->motor_position[point]
									- min_position) * 370) / difference));
		}
		fprintf(f, "\" />");
		fprintf(f,
				"<polyline fill=\"none\" stroke=\"#ff0000\" stroke-width=\"2\" points=\"\n");
		// x step is 0.61
		for (int i = 0; i < CYCLE_SIZE; ++i) {
			int point = ID - i;
			if (point < 0)
				point += CYCLE_SIZE;
			fprintf(f, "%f, %f\n", 700 - (i * 0.61),
					(370
							- (float) ((http_d->requested_position[point]
									- min_position) * 370) / difference));
		}
		fprintf(f, "\" />;");
		fprintf(f,
				"<g> <circle cx=\"80\" cy=\"416\" data-value=\"7.2\" r=\"4\" class=\"red\"></circle> <circle cx=\"80\" cy=\"436\" data-value=\"7.2\" r=\"4\" class=\"blue\"></circle> <text x=\"90\" y=\"420\" class=\"label-title\">actual position</text> <text x=\"90\" y=\"440\" class=\"label-title\">requested position</text> </g><style> .graph .labels.x-labels { text-anchor: middle; } .graph .labels.y-labels { text-anchor: end; } .graph .grid { stroke: #ccc; stroke-dasharray: 0; stroke-width: 1; } .labels { font-size: 14px; } .label-title { font-weight: bold; /* text-transform: uppercase; */ font-size: 12px; fill: #000; } .box-title { margin-bottom: 0; } .box-description { margin-top: 0.2rem; } .red { fill: #ff0000; } .blue { fill: #0074d9; } </style></svg>\n");

	} else if (strcmp(filename, "/pwm.svg") == 0 || strcmp(filename, "/pwm1.svg") == 0) {
		// printf("pwm.svg\n");
		/* PVM graph */
		/* Print the HTTP Response Header */
		fprintf(f, "HTTP/1.0 200 OK\r\n");
		fprintf(f, "Content-Type: image/svg+xml\r\n");

		fprintf(f, "\r\n");

		fprintf(f,
				"<svg version=\"1.2\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" class=\"graph\" aria-labelledby=\"title\" role=\"img\"> <title id=\"title\">Current PWM duty cycle</title> <g class=\"grid x-grid\" id=\"xGrid\"> <line x1=\"90\" x2=\"90\" y1=\"5\" y2=\"371\"></line> </g> <g class=\"grid y-grid\" id=\"yGrid\"> <line x1=\"90\" x2=\"705\" y1=\"370\" y2=\"370\"></line> </g> <g class=\"labels x-labels\"> <text x=\"100\" y=\"392\">-2000</text> <text x=\"250\" y=\"392\">-1500</text> <text x=\"400\" y=\"392\">-1000</text> <text x=\"550\" y=\"392\">-500</text> <text x=\"700\" y=\"392\">0</text> <text x=\"400\" y=\"430\" class=\"label-title\">time [ms]</text> </g><g class=\"labels y-labels\"> <text x=\"80\" y=\"14\">+100</text> <text x=\"80\" y=\"96.5\">+50</text> <text x=\"80\" y=\"189\">0</text> <text x=\"80\" y=\"273\">-50</text> <text x=\"80\" y=\"380\">-100</text> <text x=\"-115\" y=\"40\" class=\"label-title\" transform=\"rotate(-90)\">PWM duty cycle [%]</text> </g>");
		
		fprintf(f,
					"<polyline fill=\"none\" stroke=\"#0074d9\" stroke-width=\"2\" points=\"\n");
			for (int i = 0; i < CYCLE_SIZE; ++i) {
				int point = ID - i;
				if (point < 0)
					point += CYCLE_SIZE;
				fprintf(f, "%f, %f\n", 700 - (i * 0.61),
						(float) (http_d->pwm_speed[point] * 1.85) + 185);
			}
			fprintf(f, "\" /><style> .graph .labels.x-labels { text-anchor: middle; } .graph .labels.y-labels { text-anchor: end; } .graph .grid { stroke: #ccc; stroke-dasharray: 0; stroke-width: 1; } .labels { font-size: 14px; } .label-title { font-weight: bold; /* text-transform: uppercase; */ font-size: 12px; fill: #000; } .box-title { margin-bottom: 0; } .box-description { margin-top: 0.2rem; } .red { fill: #ff0000; } .blue { fill: #0074d9; } </style> </svg>");
			
	} else if (strcmp(filename, "/favicon.ico") == 0) {
		printf("Favicon\n");
	}
	else {
		/* index.html */
		/* Print the HTTP Response Header */
		printf("index.html\n");
		fprintf(f, "HTTP/1.0 200 OK\r\n");
		fprintf(f, "Content-Type: text/html\r\n");

		fprintf(f, "\r\n");

		fprintf(f,
				"<!DOCTYPE html> <html lang=\"cs\"> <head> <meta charset=\"UTF-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> <title>Motor Control - Dashboard</title> </head> <body onload=\"delay()\"> <h1>Dashboard a motor control</h1> <div class=\"box\"> <h2 class=\"box-title\">Actual and Requested Motor Position</h2> <p class=\"box-description\">absolute value</p> <div class=\"box-graph\"> <img src=\"motor.svg\" class=\"graph\" name=\"motor\" /> </div> </div> <div class=\"box\"> <h2 class=\"box-title\">Current PWM duty cycle</h2> <p class=\"box-description\">in range –100%, +100%</p> <div class=\"box-graph\"> <img src=\"pwm.svg\" class=\"graph\" name=\"pwm\" /> </div> </div> <script> function sleep(ms) { return new Promise(resolve => setTimeout(resolve, ms)); } async function delay() { while (1) { if (!document.images) return; document.images['motor'].src = 'motor.svg'; await sleep(100); document.images['pwm'].src = 'pwm.svg'; await sleep(100); document.images['motor'].src = 'motor1.svg'; await sleep(100); document.images['pwm'].src = 'pwm1.svg'; await sleep(100); console.log(\"NOW\"); } } </script> </body> <style> .graph { height: 500px; width: 800px; } </style> </html>\n");
	}


	fclose(f);
	close(newFd);

}
