/* includes */

#include "vxWorks.h"
#include "HTTP.h"
#include "UDP.h"
#include <taskLib.h>

#define WEB_SERVER_PRIORITY 100

void start(char *ipAddress, int port) {

	/* Run web server to testing */
	char taskName[10];
	sprintf(taskName, "tWebServer");
	TASK_ID tWebServer = taskSpawn(taskName, WEB_SERVER_PRIORITY, 0, 4096,
			(FUNCPTR) www, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	if (tWebServer == NULL) {
		fprintf(stderr, "ERROR: Can't spawn a web server [dkm.c]\n");
	}

	/* Test the UDP communication */
	/* Init UDP strucutre */
	UDP *udp = initUDP(ipAddress, port);
	if (udp == NULL) {
		fprintf(stderr, "ERROR: Can't initialize a UDP structure! [dkm.c]\n");
		exit(1);
	}
	/* Print information about UDP strucutre */
	printf("\n~~~~~~~~~~~~~~~~~~~~~~~\n");
	if (udp->isMaster) {
		printf("MASTER MODE\n");
		printf("target IP address: %s\n", ipAddress);
	} else {
		printf("SLAVE MODE\n");
	}
	printf("communication port: %d\n", port);
	printf("~~~~~~~~~~~~~~~~~~~~~~~\n\n");

	/* Allocate structure testing sending data */
	int *data = NULL;
	data = (int*) malloc(sizeof(int) * 10);
	if (data == NULL) {
		fprintf(stderr,
				"ERROR: Can't allocated structure for testing data! [dkm.c]\n");
		exit(1);
	}

	/* Sent testing data */
	uint8_t i = 10;
	while (i != 0) {
		if (udp->isMaster) {
			for (uint8_t j = 0; j < 10; ++j) {
				data[j] = j;
			}
			sendUDPData(udp, data, 10);
			taskDelay(1000);
		} else {
			recieveUDPData(udp, data, 10);
			for (uint8_t j = 0; j < 10; ++j) {
				printf("%d ", data[j]);
			}
			printf("\n");
		}
		--i;
	}
	printf("THE END OF FILE\n");

}
