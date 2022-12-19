#include "motor.h"

SEM_ID isrSemaphore;

uint8_t A,B,A_last=0,B_last=0;
int steps;


LOCAL VXB_FDT_DEV_MATCH_ENTRY psrMotorMatch[] = {
    {"cvut,psr-motor", NULL},
    {} /* Empty terminated list */
};

void motorISR(struct psrMotor *pMotor){
	// Read the interrupt register to acknowledge reception of the interupt
	GPIO_INT_STAT(pMotor) = pMotor->gpioIrqBit;
	if(FPGA_SR(pMotor) & FPGA_SR_IRC_B_MON){A=1;}else{A=0;}
	if(FPGA_SR(pMotor) & FPGA_SR_IRC_A_MON){B=1;}else{B=0;}
	if((A==1 && A_last==1 && B_last==1 && B==0)||
					(A==0 && A_last==0 && B_last==0 && B==1)||
					(B==1 && B_last==1 && A_last==0 && A==1)||
					(B==0 && B_last==0 && A_last==1 && A==0)){
					steps++;
				 }
				 else if(
					(A==1 && A_last==1 && B_last==0 && B==1)||
					(A==0 && A_last==0 && B_last==1 && B==0)||
					(B==1 && B_last==1 && A_last==1 && A==0)||
					(B==0 && B_last==0 && A_last==0 && A==1)){
					 steps--;
				 	 }	
				A_last=A; B_last=B;		
	semGive(isrSemaphore);
}
void printingTask(struct psrMotor *pMotor){
	printf("service task started #motor.c\n");
	while(1){
		semTake(isrSemaphore,WAIT_FOREVER);
		printf("kroky %d #motor.c\n",steps);
		
		}
}
void motorShutdown(struct psrMotor *pMotor){
	//disable motor interrupt
	GPIO_INT_DIS(pMotor) |= pMotor->gpioIrqBit;
}
LOCAL STATUS motorProbe(VXB_DEV_ID pInst)
{
        if (vxbFdtDevMatch(pInst, psrMotorMatch, NULL) == ERROR)
                return ERROR;
        VXB_FDT_DEV *fdtDev = vxbFdtDevGet(pInst);
        /* Tell VxWorks which of two motors we want to control */
        if (strcmp(fdtDev->name, "pmod1") == 0)
                return OK;
        else
                return ERROR;
}
LOCAL STATUS motorAttach(VXB_DEV_ID pInst)
{
    struct psrMotor *pMotor;

    VXB_RESOURCE *pResFPGA = NULL;
    VXB_RESOURCE *pResGPIO = NULL;
    VXB_RESOURCE *pResIRQ = NULL;

    pMotor = (struct psrMotor *)vxbMemAlloc(sizeof(struct psrMotor));
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

    pMotor->fpgaRegs = ((VXB_RESOURCE_ADR *)(pResFPGA->pRes))->virtAddr;
    pMotor->gpioRegs = ((VXB_RESOURCE_ADR *)(pResGPIO->pRes))->virtAddr;

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
    GPIO_DIR(pMotor) &= ~pMotor->gpioIrqBit;     /* set GPIO as input */
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
int getMotorSteps(struct psrMotor *pMotor){
	return steps;
}
void moveMotor(struct psrMotor *pMotor,int input_steps){
	int wanted_steps=input_steps+steps;
	//TODO absolutni rozdil steps  A input_steps+start_steps
	// jak daleko jsou steps od wanted steps
	FPGA_PWM_DUTY(pMotor)&= FPGA_PWM_PERIOD_MASK;//vymaze smer
	FPGA_PWM_DUTY(pMotor)&= ~FPGA_PWM_PERIOD_MASK;//vymaze duty
	FPGA_PWM_DUTY(pMotor)|=0x200;//very slow rotation
	printf("steps %d , wanted_steps %d \n",steps,wanted_steps);
	while(steps!=wanted_steps){
		
		
		//nastav smer		
		if(steps>wanted_steps){
			FPGA_PWM_DUTY(pMotor)|=0x1<<31;//Smer1
		}
		if(steps<wanted_steps){
			FPGA_PWM_DUTY(pMotor)|=0x1<<30;//Smer2
			}
		printf("steps %d , wanted_steps %d \n",steps,wanted_steps);
		}
	
	FPGA_PWM_DUTY(pMotor)=0;//kill rotation
}
void motorInit(){
	VXB_DEV_ID motorDrv = vxbDevAcquireByName("pmod1", 0);
	isrSemaphore=semBCreate(SEM_Q_FIFO,0);
	if (motorDrv == NULL) {
	        fprintf(stderr, "vxbDevAcquireByName failed\n");
	        return;
	    }
	
	printf("acquired\n");
	motorProbe(motorDrv);
	printf("motor probed\n");
	motorAttach(motorDrv);
	printf("motor attatched\n");
	struct psrMotor *pMotor = vxbDevSoftcGet(motorDrv);
	FPGA_PWM_PERIOD(pMotor)|=0xA00;
	FPGA_CR(pMotor)|=FPGA_CR_PWM_EN;//enable pwm generator 
	FPGA_PWM_DUTY(pMotor)|=0x00000500;//motor to 8forward 4 to revers than duty
	//FPGA_PWM_DUTY(pMotor)|=0x1<<31;//Smer1
	//FPGA_PWM_DUTY(pMotor)|=0x1<<30;//Smer2
			
	
	TASK_ID service_task = taskSpawn("tService", 0, 0, 4096, (FUNCPTR) printingTask, pMotor, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	printf("initialization done \n");
	moveMotor(pMotor,3000);
	while(1){
			char c =(char)getchar();
			if(c=='q'){break;}
		}
	printf("motor shutdown\n");
	//taskDelete(motorService);
	motorShutdown(pMotor);
}

