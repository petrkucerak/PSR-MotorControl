/*
 * UDP.h
 *
 *  Created on: Dec 7, 2022
 *      Author: kucerp28
 */

#ifndef UDP_H_
#define UDP_H_

#include <stdbool.h>
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

/**
 * @brief Structure defines variables using for initializing packers
 * and determine a program mode (Slave or Master).
 * 
 */
typedef struct udp_s {
	bool isMaster;

	int sockd;
	struct sockaddr_in my_addr, srv_addr, my_name, cli_name;
	int count, status;
	socklen_t addrlen;
} UDP;

/* Function definitions */

/**
 * @brief Initialization structure for saving data about UDP protocol
 * and ip socket. If UDP protocol should running in Master mode, please
 * specify the variable with IP address. If should running in Slave mode,
 * set up NULL to IP variable.
 * 
 * @param ipAddress [char] string with target IP address
 * @param port [int] number with target IP protocols 
 * @return [UDP] structure UDP
 */
UDP initUDP(char *ipAddress, int port);

/**
 * @brief This function sending data with defined size to target
 * defined in udp structure.
 * 
 * @param udp [UDP] structure UDP
 * @param data TODO
 * @param size TODO
 */
void sendUDPData(UDP *udp, int *data, int size);

/**
 * @brief This function receiving data with defined size
 * by definition in udp structure.
 * 
 * @param udp [UDP] structure UDP
 * @param data TODO
 * @param size TODO
 */
void recieveUDPData(UDP *udp, int *data, int size);

/**
 * @brief Free a memory allocated for UDP structure
 * and close initialized socket.
 * 
 * @param udp [UDP] udp structure to deleted
 */
void closeUDP(UDP *udp);

#endif /* UDP_H_ */
