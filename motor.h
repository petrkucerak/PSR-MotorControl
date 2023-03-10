/**
 * @file motor.h
 * @author Jan Tonner (tonnejan@cvut.fel.cz)
 * @brief Header file for motor control, including macros for accessing GPIO
 * registers
 * @version 0.1
 * @date 2023-01-06
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef MOTOR_H_
#define MOTOR_H_

#include "UDP.h"
#include "vxWorks.h"
#include <hwif/buslib/vxbFdtLib.h>
#include <hwif/vxBus.h>
#include <semLib.h>
#include <stdio.h>
#include <subsys/int/vxbIntLib.h>
#include <sysLib.h>
#include <taskLib.h>
// #include <string.h>
//  GPIO registers (TRM sect. 14 and B.19)
#define GPIO_DATA_RO_OFFSET 0x068
#define GPIO_DIRM_OFFSET 0x284
#define GPIO_INT_EN_OFFSET 0x290
#define GPIO_INT_DIS_OFFSET 0x294
#define GPIO_INT_STATUS_OFFSET 0x298
#define GPIO_INT_TYPE_OFFSET 0x29c
#define GPIO_INT_POL_OFFSET 0x2a0
#define GPIO_INT_ANY_OFFSET 0x2a4

// FPGA registers
// (https://rtime.felk.cvut.cz/psr/cviceni/semestralka/#fpga-registers)
#define FPGA_CR_OFFSET 0x0
#define FPGA_CR_PWM_EN 0x40

#define FPGA_SR_OFFSET 0x0004
#define FPGA_SR_IRC_A_MON 0x100
#define FPGA_SR_IRC_B_MON 0x200
#define FPGA_SR_IRC_IRQ_MON 0x400

#define FPGA_PWM_PERIOD_OFFSET 0x0008
#define FPGA_PWM_PERIOD_MASK 0x3fffffff

#define FPGA_PWM_DUTY_OFFSET 0x000c
#define FPGA_PWM_DUTY_MASK 0x3fffffff
#define FPGA_PWM_DUTY_DIR_A 0x40000000
#define FPGA_PWM_DUTY_DIR_B 0x80000000

#define REGISTER(base, offs) (*((volatile UINT32 *)((base) + (offs))))

#define FPGA_CR(motor) REGISTER((motor)->fpgaRegs, FPGA_CR_OFFSET)
#define FPGA_SR(motor) REGISTER((motor)->fpgaRegs, FPGA_SR_OFFSET)
#define FPGA_PWM_PERIOD(motor)                                                 \
   REGISTER((motor)->fpgaRegs, FPGA_PWM_PERIOD_OFFSET)
#define FPGA_PWM_DUTY(motor) REGISTER((motor)->fpgaRegs, FPGA_PWM_DUTY_OFFSET)
#define GPIO_DIR(motor) REGISTER((motor)->gpioRegs, GPIO_DIRM_OFFSET)
#define GPIO_INT_STAT(motor) REGISTER((motor)->gpioRegs, GPIO_INT_STATUS_OFFSET)
#define GPIO_INT_EN(motor) REGISTER((motor)->gpioRegs, GPIO_INT_EN_OFFSET)
#define GPIO_INT_DIS(motor) REGISTER((motor)->gpioRegs, GPIO_INT_DIS_OFFSET)
#define GPIO_INT_TYPE(motor) REGISTER((motor)->gpioRegs, GPIO_INT_TYPE_OFFSET)
#define GPIO_INT_POL(motor) REGISTER((motor)->gpioRegs, GPIO_INT_POL_OFFSET)
#define GPIO_INT_ANY(motor) REGISTER((motor)->gpioRegs, GPIO_INT_ANY_OFFSET)
#define GPIO_RAW(motor) REGISTER((motor)->gpioRegs, GPIO_DATA_RO_OFFSET)

#define MOTOR_PWM_PERIOD 0xA00

typedef struct encoder_info {
   uint8_t A, B, A_last, B_last;
} encoder_info;

static int steps;

/**
 * @brief Predefined structure for accessing motor registers
 *
 */
struct psrMotor {
   VIRT_ADDR fpgaRegs;
   VIRT_ADDR gpioRegs;
   UINT32 gpioIrqBit;
};
/**
 * @brief Inicialization of motor and all of its functions
 * mainly motor interrupt service
 *
 * @return Struct psrMotor* is handle for all comands for the motor
 * mainly move and shutdown
 */
struct psrMotor *motorInit();
/**
 * @brief Ending command for shutting down the motor and doing
 * all the required things for correct shutdown
 * @param pMotor Pointer to motor structure
 */
void motorShutdown(struct psrMotor *pMotor);
/**
 * @brief Command for the motor to move to position relative
 * to start position. Proporsion conrolled speed.
 *
 * @param pMotor Handler for the motor we want to move
 * @param input_steps Position relative to the starting position
 */
void moveMotor(struct psrMotor *pMotor, int wanted_steps);
/**
 * @brief Task for controlling motor movement and also position hold
 *
 * @param pMotor pointer to motor which i want to controll
 * @param udp pointer to udp struct needed for mutex
 */
void motorControllTask(struct psrMotor *pMotor, UDP *udp, int *end);
/**
 * @brief Get the actual step count of the motor since init
 *
 * @return Int count of steps relative to start
 */
int getMotorSteps();
/**
 * @brief Get actual value of motor pwm with minus as oposite direction
 *
 * @param pMotor pointer to motor which i want to read from
 * @return precentage value of pwm
 */
int getPWM(struct psrMotor *pMotor);
/**
 * @brief Set motor steps to zero, reinicialize the step counter
 *
 */
void resetSteps();

#endif /* MOTOR_H_*/
