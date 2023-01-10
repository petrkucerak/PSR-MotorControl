/**
 * @file dkm.c
 * @author Petr Kucera (kucerp28@fel.cvut.cz), Jan Tonner (tonnejan@cvut.fel.cz)
 * @brief The main file which contains the start function.
 * @version 0.1
 * @date 2023-01-10
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "HTTP.h"
#include "UDP.h"
#include "motor.h"
#include "vxWorks.h"
#include <semLib.h>
#include <taskLib.h>
#include <time.h>

/**
 * @brief The task priority of the web server.
 *
 */
#define WEB_SERVER_PRIORITY 100
/**
 * @brief The task priority of the motor control.
 *
 */
#define MOTOR_CONTROLL_PRIORITY 100

/**
 * @brief The task priority of the HTTP data handler for sampling data for the
 * web server task.
 *
 */
#define HTTP_DATA_HANDLER_PRIORITY 120

/**
 * @brief The task priority of the UDP handler data for handling incoming data.
 *
 */
#define UDP_HANDLER_PRIORITY 100

/**
 * @brief The task priority of the motor controlling for the master task.
 *
 */
#define MOTOR_MASTER_PRIORITY 100

/**
 * @brief The size of the UDP packet with data. The minimal size is 4 bytes. If
 * a message is shorter, the protocol will fail.
 *
 */
#define UDP_SIZE 10

/**
 * @brief The global variable for the HTTP data structure.
 *
 */
static HTTP_D *http_d;

/**
 * @brief The global variable for the UDP data structure.
 *
 */
static UDP *udp;

/**
 * @brief The global variable for the motor data structure.
 *
 */
static struct psrMotor *my_motor;

/**
 * @brief The global variable for the signalizing program end.
 *
 */
static int isEnd;

/**
 * @brief The pointer to the global variable for the signalizing program end.
 *
 */
static int *isEndp;

/**
 * @brief Real-time program starting function. The function determines 'master'
 * or 'slave' mode by parameters.
 *
 * @param ipAddress An empty argument means that the program should run in slave
 * mode. Specify the target (slave) IP address in master mode.
 * @param port Master and Slave ports must be the same.
 */
void start(char *ipAddress, int port)
{

   /* Set ending status to negative */
   isEnd = 0;
   isEndp = &isEnd;

   /* Allocate memory for HTTP data structure and create http mutex*/
   http_d = initHTTPData();
   if (http_d == NULL) {
      fprintf(stderr, "ERROR: Can't initialize a HTTP_D structure! [dkm.c]\n");
      exit(1);
   }

   /* Init UDP structure */
   udp = initUDP(ipAddress, port);
   if (udp == NULL) {
      fprintf(stderr, "ERROR: Can't initialize a UDP structure! [dkm.c]\n");
      exit(1);
   }

   /* Print information about UDP structure */
   printf("\n~~~~~~~~~~~~~~~~~~~~~~~\n");
   if (udp->isMaster) {
      printf("MASTER MODE\n");
      printf("target IP address: %s\n", ipAddress);
   } else {
      printf("SLAVE MODE\n");
   }
   printf("communication port: %d\n", port);
   printf("~~~~~~~~~~~~~~~~~~~~~~~\n\n");

   /* Allocate data for testing */
   int *data = NULL;
   data = (int *)malloc(sizeof(int) * UDP_SIZE);
   if (data == NULL) {
      fprintf(stderr,
              "ERROR: Can't allocated structure for testing data! [UDP.c]\n");
      exit(1);
   }

   /* Run motor handler */
   my_motor = motorInit();

   /* Spawn tasks */
   char taskName[20];
   if (udp->isMaster) {

      /* MASTER */
      sprintf(taskName, "tMotorMaster");
      TASK_ID tMotorMaster = taskSpawn(
          taskName, MOTOR_MASTER_PRIORITY, 0, 4096, (FUNCPTR)motorMaster,
          (UDP *)udp, (int)UDP_SIZE, isEndp, 0, 0, 0, 0, 0, 0, 0);
      if (tMotorMaster == NULL) {
         fprintf(stderr,
                 "ERROR: Can't spawn the master motor control task [dkm.c]\n");
      }

   } else {
      /* SLAVE */
      /* Start web server as a task */
      sprintf(taskName, "tWebServer");
      TASK_ID tWebServer =
          taskSpawn(taskName, WEB_SERVER_PRIORITY, 0, 4096, (FUNCPTR)www,
                    isEndp, (HTTP_D *)http_d, 0, 0, 0, 0, 0, 0, 0, 0);
      if (tWebServer == NULL) {
         fprintf(stderr, "ERROR: Can't spawn a web server [dkm.c]\n");
      }
      /* Init UDP HANDLER task - save wanted position into the udp structure*/
      sprintf(taskName, "tUDPHandler");
      TASK_ID tUDPHandler = taskSpawn(
          taskName, UDP_HANDLER_PRIORITY, 0, 4096, (FUNCPTR)UDPHandler,
          (UDP *)udp, (int)UDP_SIZE, isEndp, 0, 0, 0, 0, 0, 0, 0);
      if (tUDPHandler == NULL) {
         fprintf(stderr, "ERROR: Can't spawn a UDP handler [dkm.c]\n");
      }

      /* Init HTTP DATA HANDLER - periodically saving data from motor and udp
       * structure*/
      sprintf(taskName, "tHTTPDHandler");
      TASK_ID tHTTPDHandler = taskSpawn(
          taskName, HTTP_DATA_HANDLER_PRIORITY, 0, 4096,
          (FUNCPTR)handleHTTPData, (UDP *)udp, (struct psrMotor *)my_motor,
          (HTTP_D *)http_d, isEndp, 0, 0, 0, 0, 0, 0);
      if (tHTTPDHandler == NULL) {
         fprintf(stderr, "ERROR: Can't spawn a HTTP Data handler [dkm.c]\n");
      }
      /* Init MOTOR CONTROL task - rotate motor to udp wanted position */
      sprintf(taskName, "tMotorControl");
      TASK_ID tMotorControl =
          taskSpawn(taskName, MOTOR_CONTROLL_PRIORITY, 0, 4096,
                    (FUNCPTR)motorControllTask, (struct psrMotor *)my_motor,
                    (UDP *)udp, isEndp, 0, 0, 0, 0, 0, 0, 0);
      if (tMotorControl == NULL) {
         fprintf(stderr, "ERROR: Can't spawn a Motor control [dkm.c]\n");
      }
   }
   /* Handle q or Q symbols to terminate all process */
   while (!*isEndp) {
      char c = (char)getchar();
      if (c == 'q' || c == 'Q')
         isEnd = 1;
   }
   printf("\n~~~~~~~~~~~~~~~~~~~~~~~\n");
   printf("Start the Finish routine!\n");
   removeHTTPData(http_d);
   printf("HTTP data has been removed\n");
   closeUDP(udp);
   printf("UDP structure has been removed\n");
   motorShutdown(my_motor);
   printf("Motor has been stopped\n");
   printf("\n~~~~~~~~~~~~~~~~~~~~~~~\n");
   printf("THE END OF FILE\n");
}
