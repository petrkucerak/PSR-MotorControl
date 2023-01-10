/**
 * @file HTTP.h
 * @author Petr Kucera (kucerp28@fel.cvut.cz)
 * @brief The header file defines functions for sampling data into the HTTP data
 * structure and serving the web server.
 * @version 0.1
 * @date 2023-01-10
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef HTTP_H_
#define HTTP_H_

#include "UDP.h"
#include "motor.h"
#include <semLib.h>

/**
 * @brief The macro defines the HTTP data length.
 *
 * Note: The graphs should show at least 2 seconds
 * of history with time resolution ≤ 5 ms.
 *
 * We want delay 2 ms -> CYCLE SIZE => 1000
 *
 */
#define CYCLE_SIZE 1000

/**
 * @brief The structure for sampling data. It is necessary for graphs on the web
 * server.
 *
 */
typedef struct http_s {
   /**
    * @brief Mutex for protecting writing to this structure.
    *
    */
   SEM_ID http_sem;

   /**
    * @brief Last n motor position values.
    *
    */
   int motor_position[CYCLE_SIZE];
   /**
    * @brief Last n requested motor position values.
    *
    */
   int requested_position[CYCLE_SIZE];

   /**
    * @brief Last n pwm intensity (speed) values.
    *
    */
   int pwm_speed[CYCLE_SIZE];

   /**
    * @brief The value that points to the most recent value's index.
    *
    */
   int cycle_p;
} HTTP_D;

/**
 * @brief The function of running the web server. Web server spawn HTTP
 * responses as separate tasks.
 *
 * @param isEndp The pointer to the global variable for the signalizing program
 * end.
 * @param http_d The pointer to the HTTP data.
 */
void www(int *isEndp, HTTP_D *http_d);

/**
 * @brief The server response renders graphs with motor positions and PWA
 * intensity.
 *
 * @param newFd Web server response file descriptor.
 * @param http_d The pointer to the HTTP data.
 */
void serverResponse(int newFd, HTTP_D *http_d);

/**
 * @brief The function allocates the HTTP data structure and sets initialization
 * values.
 *
 * @return HTTP_D*
 */
HTTP_D *initHTTPData();

/**
 * @brief The function is sampling data for rendering graphs with a specific
 * period.
 *
 * @param udp The pointer to the UDP structure.
 * @param my_motor The pointer to the motor structure.
 * @param http_d The pointer to the http data strucure.
 * @param isEndp The pointer to the global variable for the signalizing program
 * end.
 */
void handleHTTPData(UDP *udp, struct psrMotor *my_motor, HTTP_D *http_d,
                    int *isEndp);

/**
 * @brief The local function for saving data into the HTTP Data structure with a
 * mutex to prevent damaged data.
 *
 * @param http_d The pointer to the http data strucure.
 * @param motor_position Value of motor position
 * @param requested_position Value of requested motor position
 * @param pwm_speed Value of pwm intesity (speed)
 */
void saveHTTPData(HTTP_D *http_d, int motor_position, int requested_position,
                  int pwm_speed);

/**
 * @brief Removes allocated data for the HTTP data structure.
 *
 * @param http_d The pointer to the http data strucure.
 */
void removeHTTPData(HTTP_D *http_d);

#endif /* UDP_H_ */
