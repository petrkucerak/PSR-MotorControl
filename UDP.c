/*
 * UDP.c
 *
 *  Created on: Dec 7, 2022
 *      Author: kucerp28
 */

#include "UDP.h"

#include "vxWorks.h"
#include <stdio.h>
#include <string.h>
#include <inetLib.h>
#include <sockLib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

UDP *initUDP(char *ipAddress, int port) {

	UDP *udp = NULL;
	udp = (UDP*) malloc(sizeof(UDP));
	if (udp == NULL) {
		fprintf(stderr,
				"ERROR: Can't allocate memory for the UDP structure! [UDP.C]\n");
		exit(1);
	}
	if (ipAddress[0] == '\0') {
		/* SLAVE */
		udp->isMaster = false;

		/* Create a UDP socket */
		udp->sockd = socket(AF_INET, SOCK_DGRAM, 0);
		if (udp->sockd == -1) {
			perror("Socket creation error");
			exit(1);
		}

		/* Configure server address */
		udp->my_name.sin_family = AF_INET;
		udp->my_name.sin_addr.s_addr = INADDR_ANY;
		udp->my_name.sin_port = (uint16_t) port;

		udp->status = bind(udp->sockd, (struct sockaddr*) &udp->my_name,
				sizeof(udp->my_name));
	} else {
		/* MASTER */
		udp->isMaster = true;

		/* Create a UDP socket */
		udp->sockd = socket(AF_INET, SOCK_DGRAM, 0);
		if (udp->sockd == -1) {
			fprintf(stderr, "ERROR: Socket creation error! [UDP.c]\n");
			exit(1);
		}

		/* Configure client address */
		udp->my_addr.sin_family = AF_INET;
		udp->my_addr.sin_addr.s_addr = INADDR_ANY;
		udp->my_addr.sin_port = (uint16_t) port;

		bind(udp->sockd, (struct sockaddr*) &udp->my_addr,
				sizeof(udp->my_addr));

		/* Set server address */
		udp->srv_addr.sin_family = AF_INET;
		inet_aton(ipAddress, &udp->srv_addr.sin_addr);
		udp->srv_addr.sin_port = (uint16_t) port;
	}
	return udp;
}

void sendUDPData(UDP *udp, int *data, int size) {

	/* start of debugging print */
	if (!udp->isMaster)
		printf("WARNING: Slaves can't received data! [UDP.c]\n");
	/* end of debugging print */

	/* Send data */
	int t_size = sendto(udp->sockd, data, size, 0,
			(struct sockaddr*) &udp->srv_addr, sizeof(udp->srv_addr));
	if (t_size != size) {
		fprintf(stderr,
				"ERROR: Sumcheck of send data doesn't match! [UDP.c]\n");
		exit(1);
	}
}

void recieveUDPData(UDP *udp, int *data, int size) {

	/* start of debugging print */
	if (udp->isMaster)
		printf("WARNING: Master can't sending data! [UDP.c]\n");
	/* end of debugging print */

	int t_size = recvfrom(udp->sockd, data, size, 0,
			(struct sockaddr*) &udp->cli_name, &udp->addrlen);
	if (t_size != size) {
		fprintf(stderr,
				"ERROR: Sumcheck of recieved data doesn't match! [UDP.c]\n");
		exit(1);
	}
}

void closeUDP(UDP *udp) {
	close(udp->sockd);
	free(udp);
	udp = NULL;
}
