/*
 * HTTP.h
 *
 *  Created on: Dec 14, 2022
 *      Author: kucerp28
 */

#ifndef HTTP_H_
#define HTTP_H_

#include <semLib.h>
#include "UDP.h"
#include "motor.h"

/**
 * Note: The graphs should show at least 2 seconds
 * of history with time resolution ≤ 5 ms.
 * 
 * We want delay 2 ms -> CYCLE SIZE => 1000
 */
#define CYCLE_SIZE 1000

typedef struct http_s {
	SEM_ID http_sem;

	int motor_position[CYCLE_SIZE];
	int requested_position[CYCLE_SIZE];
	int pwm_speed[CYCLE_SIZE];
	int cycle_p;
} HTTP_D;

void www(int *isEndp);
void serverResponse(int newFd);

HTTP_D* initHTTPData();
void handleHTTPData(UDP *udp, struct psrMotor *my_motor, HTTP_D *http_d,
		int *isEndp);
void removeHTTPData(HTTP_D *http_d);

#endif /* UDP_H_ */
