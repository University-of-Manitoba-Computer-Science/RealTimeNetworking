#include "gyro.h"
void accelOnlyMode(){

	//for some reason unknown to me you have to write 
	//this control register first or else writting
	//the next will get rid of the aut inc bit
	//big shrug
	uint16_t writeBuffer[WRITE_BUF_SIZE];
	writeBuffer[0] = CTRL_REG8_IF_ADD_INC_Msk;
	initWrite(CTRL_REG8,1,writeBuffer);
	
	while((SERCOM2_REGS->I2CM.SERCOM_INTFLAG) == 0U){
		
		//wait for sync of SYNCBUSY.SYSOP to ensure we are not doing anything on the bus

	}//while

	writeBuffer[0] = CTRL_REG6_XL_50HZ_Msk;
	initWrite(CTRL_REG6_XL,1,writeBuffer);

}//accelOnlyMode