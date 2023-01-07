
/**
 * @file motor.c
 * @author Jan Tonner (tonnejan@cvut.fel.cz)
 * @brief Contains implementations of functions for motor controll and data access
 * @version 0.1
 * @date 2023-01-06
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <string.h>
#include "motor.h"

SEM_ID isrSemaphore;
encoder_info enc_i;
#define e enc_i

LOCAL VXB_FDT_DEV_MATCH_ENTRY psrMotorMatch[] = { { "cvut,psr-motor", NULL },
		{ } /* Empty terminated list */
};

void motorISR(struct psrMotor *pMotor) {
	// Read the interrupt register to acknowledge reception of the interupt
	GPIO_INT_STAT(pMotor) = pMotor->gpioIrqBit;
	if (FPGA_SR(pMotor) & FPGA_SR_IRC_B_MON) {
		e.A = 1;
	} else {
		e.A = 0;
	}
	if (FPGA_SR(pMotor) & FPGA_SR_IRC_A_MON) {
		e.B = 1;
	} else {
		e.B = 0;
	}
	if ((e.A == 1 && e.A_last == 1 && e.B_last == 1 && e.B == 0)
			|| (e.A == 0 && e.A_last == 0 && e.B_last == 0 && e.B == 1)
			|| (e.B == 1 && e.B_last == 1 && e.A_last == 0 && e.A == 1)
			|| (e.B == 0 && e.B_last == 0 && e.A_last == 1 && e.A == 0)) {
		steps++;
	} else if ((e.A == 1 && e.A_last == 1 && e.B_last == 0 && e.B == 1)
			|| (e.A == 0 && e.A_last == 0 && e.B_last == 1 && e.B == 0)
			|| (e.B == 1 && e.B_last == 1 && e.A_last == 1 && e.A == 0)
			|| (e.B == 0 && e.B_last == 0 && e.A_last == 0 && e.A == 1)) {
		steps--;
	}
	e.A_last = e.A;
	e.B_last = e.B;
}
void motorShutdown(struct psrMotor *pMotor) {
	//disable motor interrupt
	GPIO_INT_DIS(pMotor) |= pMotor->gpioIrqBit;
	printf("motor shutdown done\n");
}
LOCAL STATUS motorProbe(VXB_DEV_ID pInst) {
	if (vxbFdtDevMatch(pInst, psrMotorMatch, NULL) == ERROR)
		return ERROR;
	VXB_FDT_DEV *fdtDev = vxbFdtDevGet(pInst);
	/* Tell VxWorks which of two motors we want to control */
	if (strcmp(fdtDev->name, "pmod1") == 0)
		return OK;
	else
		return ERROR;
}
LOCAL STATUS motorAttach(VXB_DEV_ID pInst) {
	struct psrMotor *pMotor;

	VXB_RESOURCE *pResFPGA = NULL;
	VXB_RESOURCE *pResGPIO = NULL;
	VXB_RESOURCE *pResIRQ = NULL;

	pMotor = (struct psrMotor*) vxbMemAlloc(sizeof(struct psrMotor));
	if (pMotor == NULL) {
		perror("vxbMemAlloc");
		goto error;
	}

	// Map memory for FPGA registers and GPIO registers
	pResFPGA = vxbResourceAlloc(pInst, VXB_RES_MEMORY, 0);
	pResGPIO = vxbResourceAlloc(pInst, VXB_RES_MEMORY, 1);
	if (pResFPGA == NULL || pResGPIO == NULL) {
		fprintf(stderr, "Memory resource allocation error\n");
		goto error;
	}

	pMotor->fpgaRegs = ((VXB_RESOURCE_ADR*) (pResFPGA->pRes))->virtAddr;
	pMotor->gpioRegs = ((VXB_RESOURCE_ADR*) (pResGPIO->pRes))->virtAddr;

	pResIRQ = vxbResourceAlloc(pInst, VXB_RES_IRQ, 0);
	if (pResIRQ == NULL) {
		fprintf(stderr, "IRQ resource alloc error\n");
		goto error;
	}

	int len;
	VXB_FDT_DEV *fdtDev = vxbFdtDevGet(pInst);
	const UINT32 *pVal = vxFdtPropGet(fdtDev->offset, "gpio-irq-bit", &len);
	if (pVal == NULL || len != sizeof(*pVal)) {
		fprintf(stderr, "vxFdtPropGet(gpio-irq-bit) error\n");
		goto error;
	}
	pMotor->gpioIrqBit = vxFdt32ToCpu(*pVal);
	printf("gpioIrqBit=%#x\n", pMotor->gpioIrqBit);

	/* Associate out psrMotor structure (software context) with the device */
	vxbDevSoftcSet(pInst, pMotor);

	// Configure GPIO
	GPIO_INT_STAT(pMotor) |= pMotor->gpioIrqBit; /* reset status */
	GPIO_DIR(pMotor) &= ~pMotor->gpioIrqBit; /* set GPIO as input */
	GPIO_INT_TYPE(pMotor) |= pMotor->gpioIrqBit; /* generate IRQ on edge */
	GPIO_INT_POL(pMotor) &= ~pMotor->gpioIrqBit; /* falling edge */
	GPIO_INT_ANY(pMotor) &= ~pMotor->gpioIrqBit; /* enable IRQ only on one pin */

	// Connect the interrupt handler
	STATUS s1 = vxbIntConnect(pInst, pResIRQ, motorISR, pMotor);
	if (s1 == ERROR) {
		fprintf(stderr, "vxbIntConnect error\n");
		goto error;
	}
	// Enable interrupts
	if (vxbIntEnable(pInst, pResIRQ) == ERROR) {
		fprintf(stderr, "vxbIntEnable error\n");
		vxbIntDisconnect(pInst, pResIRQ);
		goto error;
	}

	GPIO_INT_EN(pMotor) = pMotor->gpioIrqBit;

	return OK;

	error:
	// Free any allocated resources
	if (pResIRQ != NULL) {
		vxbResourceFree(pInst, pResIRQ);
	}
	if (pResGPIO != NULL) {
		vxbResourceFree(pInst, pResGPIO);
	}
	if (pMotor != NULL) {
		vxbMemFree(pMotor);
	}
	return ERROR;
}
void resetSteps() {
	steps = 0;
}
int getMotorSteps() {
	return steps;
}
int getPWM(struct psrMotor *pMotor) {
	int pwm = FPGA_PWM_DUTY(pMotor);
	int dir = 0;
	if (pwm & 0x1 << 31) {
		dir = 1;
	} //1dir+
	if (pwm & 0x1 << 30) {
		dir = 2;
	} //2dir-
	if (dir == 0) {
		return 0;
	}
	pwm = pwm & FPGA_PWM_PERIOD_MASK;
	pwm = (100 * pwm)/MOTOR_PWM_PERIOD;
	if(pwm>100){
		pwm=100;
	}
	if (dir == 1) {
		return pwm;
	} else {
		return -pwm;
	}
}
void moveMotor(struct psrMotor *pMotor, int wanted_steps) {
	//very slow rotation
	int tmp = 0;
	int speed = 0;
	speed = abs((wanted_steps - steps) * 10) + 100;
	if (steps > wanted_steps) {
		//printf("rotating -\n");
		tmp = 0x80000000; //dir-
		tmp = tmp + speed;
	}
	if (steps < wanted_steps) {
		//printf("rotating +\n");
		tmp = 0x40000000; //dir+
		tmp = tmp + speed;

	}
	FPGA_PWM_DUTY(pMotor) = tmp;
}
struct psrMotor* motorInit() {
	VXB_DEV_ID motorDrv = vxbDevAcquireByName("pmod1", 0);
	isrSemaphore = semBCreate(SEM_Q_FIFO, 0);
	if (motorDrv == NULL) {
		fprintf(stderr, "vxbDevAcquireByName failed\n");
		return 0;
	}
	// printf("acquired\n");
	motorProbe(motorDrv);
	// printf("motor probed\n");
	motorAttach(motorDrv);
	// printf("motor attatched\n");
	struct psrMotor *pMotor = vxbDevSoftcGet(motorDrv);
	FPGA_PWM_PERIOD(pMotor) |= MOTOR_PWM_PERIOD; //set motor frequency
	FPGA_CR(pMotor) |= FPGA_CR_PWM_EN; //enable pwm generator 
	printf("The motor initialization is done \n");
	return pMotor;

}

void motorControllTask(struct psrMotor *pMotor, UDP *udp,int *end) {
	
	FPGA_PWM_DUTY(pMotor) &= FPGA_PWM_PERIOD_MASK; //clear direction
	FPGA_PWM_DUTY(pMotor) &= ~FPGA_PWM_PERIOD_MASK; //clear duty

	while (!(*end)) {
		semTake(udp->position_wanted_sem, WAIT_FOREVER);
		int pos = udp->wanted_position;
		semGive(udp->position_wanted_sem);
		moveMotor(pMotor, pos);
		//printf("steps %d , wanted_steps %d \n", steps, pos);
	}
}
