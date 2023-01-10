/**
 * @file UDP.h
 * @author Petr Kucera (kucerp28@fel.cvut.cz)
 * @brief The header file defines functions for UDP communication and UDP
 * structure.
 * @version 0.1
 * @date 2023-01-10
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef UDP_H_
#define UDP_H_

#include <inetLib.h>
#include <sockLib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <semLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * @brief Structure defines variables used for initializing packers
 * and determine a program mode (Slave or Master).
 *
 */
typedef struct udp_s {
   /**
    * @brief Determine if the program runs in master or slave mode.
    *
    */
   bool isMaster;
   /**
    * @brief Value with the requested motor position.
    *
    */
   int wanted_position;
   /**
    * @brief Mutex for protecting writing to this structure.
    *
    */
   SEM_ID position_wanted_sem;

   int sockd;
   struct sockaddr_in my_addr, srv_addr, my_name, cli_name;
   int count, status;
   socklen_t addrlen;
} UDP;

/**
 * @brief Initialization structure for saving data about UDP protocol
 * and IP socket. If the UDP protocol should running in Master mode, please
 * specify the variable with the IP address. If should running in Slave mode,
 * set up empty string '\0' to IP variable.
 *
 * @param ipAddress String with target IP address
 * @param port Number with target IP protocols
 * @return Structure UDP
 */
UDP *initUDP(char *ipAddress, int port);

/**
 * @brief This function sends data with a defined size to target defined in the
 * UDP structure.
 *
 * @param udp The UDP structure
 * @param data Memory for sending
 * @param size The data size
 */
void sendUDPData(UDP *udp, int *data, int size);

/**
 * @brief This function receiving data with defined size
 * by definition in udp structure.
 *
 * @param udp The UDP structure
 * @param data Memory for receive data
 * @param size The data size
 */
void recieveUDPData(UDP *udp, int *data, int size);

/**
 * @brief Free a memory allocated for UDP structure
 * and close initialized socket.
 *
 * @param udp UDP structure to deleted
 */
void closeUDP(UDP *udp);

/**
 * @brief The function is handling incoming data via UDP protocol.
 *
 * @param udp UDP strucutre
 * @param size Transmitted data size
 * @param isEndp The pointer to the global variable for the signalizing program
 * end.
 */
void UDPHandler(UDP *udp, int size, int *isEndp);

/**
 * @brief The function is sending motor position data to a target that is
 * specified in the UDP structure.
 *
 * @param udp UDP strucutre
 * @param size Transmitted data size
 * @param isEndp The pointer to the global variable for the signalizing program
 * end.
 */
void motorMaster(UDP *udp, int size, int *isEndp);

#endif /* UDP_H_ */
