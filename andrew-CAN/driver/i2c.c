#include "i2c.h"

//Lazy source: From my project
void clkI2C(){
/////////36.5 Dependencies
	//36.5.1 I/O Lines
	//See portIni	
	
	//36.5.3 Clocks
	//Our I2C is connected to SERCOM2 so we need to configure the clocks for that
	//We need both core and slow generic clocks enabled
	GCLK_REGS->GCLK_PCHCTRL[SERCOM2_GCLK_ID_CORE] |= GCLK_PCHCTRL_CHEN_Msk;
	while((GCLK_REGS->GCLK_PCHCTRL[SERCOM2_GCLK_ID_CORE] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk){
		//wait for sync
	}//while	
	
	//Turn on Sercom2 in main clock
	MCLK_REGS->MCLK_APBBMASK |= MCLK_APBBMASK_SERCOM2_Msk;



}//clkI2C

void initI2C(){
	//36.6.2.1 Initialization 
	//This is done out of order and in the order of the cited source above. Not following the datasheet order
	//Ensure a clean register and set software reset bit
	SERCOM2_REGS->I2CM.SERCOM_CTRLA = SERCOM_I2CM_CTRLA_SWRST_Msk;
	
	while((SERCOM2_REGS->I2CM.SERCOM_SYNCBUSY) != 0U){
	
		//wait for sync of SYNCBUSY.SWRST specifically

	}//while

	//We are lazy and want to enable smart mode to do auto ACK or NACK responses when DATA.DATA is read 36.6.3.2 Smart Mode

	SERCOM2_REGS->I2CM.SERCOM_CTRLB = SERCOM_I2CM_CTRLB_SMEN_Msk;
	
	while((SERCOM2_REGS->I2CM.SERCOM_SYNCBUSY) != 0U){
		
		//wait for sync of SYNCBUSY.SYSOP probably... The sample code does this
		//but the data sheet pg 979 only mentions this being set to 1 when
		//CTRLB.CMD is set, along with a few other registers that we didnt touch yet. 

	}//while
	

	//Set our BAUD rate 
	SERCOM2_REGS->I2CM.SERCOM_BAUD = I2C_BAUD; 
	
	//We will now turn on SERCOM2 in I2C host mode
	SERCOM2_REGS->I2CM.SERCOM_CTRLA = SERCOM_I2CM_CTRLA_ENABLE_Msk | SERCOM_I2CM_CTRLA_MODE_I2C_MASTER | SERCOM_I2CM_CTRLA_SPEED_STANDARD_AND_FAST_MODE | SERCOM_I2CM_CTRLA_SCLSM(1UL);
	
	
	while((SERCOM2_REGS->I2CM.SERCOM_SYNCBUSY) != 0U){
		
		//wait for sync of SYNCBUSY.SYSOP probably...
		//But more importantly we are waiting for SYNCBUSY.ENABLE

	}//while
	
	//We now need to change the state of the SERCOM FSM as page on page 932 fig 36-4
	SERCOM2_REGS->I2CM.SERCOM_STATUS = (uint16_t)SERCOM_I2CM_STATUS_BUSSTATE(0X01UL);


	while((SERCOM2_REGS->I2CM.SERCOM_SYNCBUSY) != 0U){
		
		//wait for sync of SYNCBUSY.SYSOP as we changed STATUS.BUSSTATE 

	}//while
	
	//let us enable all interrupts so we can check for error, and who is owner of the bus
	SERCOM2_REGS->I2CM.SERCOM_INTENSET = (uint8_t)SERCOM_I2CM_INTENSET_Msk;



}//initI2C

void sercom2Init(){
    //I2C SDA on PORT_PA12
    PORT_REGS->GROUP[0].PORT_PINCFG[12] |= PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[0].PORT_PMUX[6] |= PORT_PMUX_PMUXE_C;

    //I2C SCL on PORT_PA13
    PORT_REGS->GROUP[0].PORT_PINCFG[13] |= PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[0].PORT_PMUX[6] |= PORT_PMUX_PMUXO_C;

}//sercom2Init


//Reference:
// ALL I2C Code took heavy influence from microchip's git code
// Heavy influence from https://microchip-mplab-harmony.github.io/reference_apps/apps/sam_e51_cnano/same51n_low_power_with_oled_c_click/readme.html
//------------------------------------------------------
// i2cSndAddr
//
// PURPOSE:	This code sends the address of the i2c device 
//		shifted over and writes the required r/w bit
//------------------------------------------------------
void i2cSndAddr(uint8_t addr, bool dir){
	
	volatile uint32_t regVal = (((uint32_t)(addr)) <<1U);
	
	if(dir){
		//append a 1 to the regVal to do a read
		regVal = (regVal |(uint32_t)(1UL));
	}//if
	else{
		
		regVal = (regVal |(uint32_t)(0UL));
	
	}//else
	
	// implied else its a write and the 0 is fiiiine.
	
	SERCOM2_REGS->I2CM.SERCOM_ADDR = regVal;

	while((SERCOM2_REGS->I2CM.SERCOM_SYNCBUSY) != 0U){
		
		//wait for sync of SYNCBUSY.SYSOP as we changed SERCOM.ADDR

	}//while

}//i2cSndAddr


//Reference:
// From my project in COMP4550
// ALL I2C Code took heavy influence from microchip's git code
// Heavy influence from https://microchip-mplab-harmony.github.io/reference_apps/apps/sam_e51_cnano/same51n_low_power_with_oled_c_click/readme.html
//------------------------------------------------------
// initTx
//
// PURPOSE:	This code initiates the actual transaction
// INPUT PARAMETERS:
//	addr is the address of the REGISTER, bytes is how many bytes are being read/written, 
//	data is a pointer to a buffer for reading/writing, dir is the direction of the tx READ(true) WRITE(false)
// OUTPUT:
//	A I2C tx will be sent across the SERCOM bus
//------------------------------------------------------
void initTx(uint8_t addr, size_t bytes,volatile unsigned char *data, bool dir){
	
	//make sure we are set to send ACKs not NAKCs if we previously transmitted
	SERCOM2_REGS->I2CM.SERCOM_CTRLB &= ~SERCOM_I2CM_CTRLB_ACKACT_Msk;

	//Send gyro bus address wait happens in the function
	//In order to do a any tx type we first need to do a write request to write the data of the SUB address
	//See page 29 of the LSM9DS manual for more details on read/write operations
	i2cSndAddr(GYRO_I2C_AG, WRITE);
	
	//Currently we have sent the ST, then SAD+W as required. We should get an ACK back (SAK) 

	while((SERCOM2_REGS->I2CM.SERCOM_SYNCBUSY) != 0U){
		
		//wait for sync of SYNCBUSY.SYSOP as we changed SERCOM.ADDR
	
	}//while
	
	while(SERCOM2_REGS->I2CM.SERCOM_INTFLAG == 0){
		
		//wait until we have bus again
		
	}//while

	//Now we send the address as data to the gyro
	//we DO NOT shift this one as we can use the MSB to set auto increment
	SERCOM2_REGS->I2CM.SERCOM_DATA = addr;
	
	while((SERCOM2_REGS->I2CM.SERCOM_SYNCBUSY) != 0U){
		
		//wait for sync of SYNCBUSY.SYSOP as we changed SERCOM.ADDR

	}//while
	
	

}//initTx

void initRead(uint8_t addr, size_t bytes,volatile unsigned char *data){
	volatile size_t count = 0;
	
	initTx(addr,bytes,data,READ);
	
	//exaimine page 29 of the LSM9DS1 datasheet.
	//We need to send the gyro address again with a re-start (SR then SAD+R)  
	i2cSndAddr(GYRO_I2C_AG, READ);

		while((SERCOM2_REGS->I2CM.SERCOM_INTFLAG) == 0U){
		
			//wait for sync of SYNCBUSY.SYSOP to ensure we are not doing anything on the bus

		}//while
	while(count < bytes){
		
		while((SERCOM2_REGS->I2CM.SERCOM_INTFLAG) == 0U){
		
			//wait for sync of SYNCBUSY.SYSOP to ensure we are not doing anything on the bus

		}//while
		while(SERCOM2_REGS->I2CM.SERCOM_SYNCBUSY !=0U){

		}//while
		 data[count] = (uint8_t)SERCOM2_REGS->I2CM.SERCOM_DATA;

		count ++;

	}//while
	
	if(count == bytes){
		//send a NACK and a stop signal(0x3) pg 970-971 table 36-4 we are at the end of our read.
		SERCOM2_REGS->I2CM.SERCOM_CTRLB |= SERCOM_I2CM_CTRLB_ACKACT_Msk | SERCOM_I2CM_CTRLB_CMD(3UL);
		

		while((SERCOM2_REGS->I2CM.SERCOM_SYNCBUSY) != 0U){
		
			//wait for sync of SYNCBUSY.SYSOP as we changed SERCOM.CTRLB

		}//while
	}//if
	

}//initRead


//Reference:
// ALL I2C Code took heavy influence from microchip's git code
// Heavy influence from https://microchip-mplab-harmony.github.io/reference_apps/apps/sam_e51_cnano/same51n_low_power_with_oled_c_click/readme.html
//------------------------------------------------------
// initWrite
//
// PURPOSE:	This code initiates the write transaction for the gyro
// INPUT PARAMETERS:
//	addr is the address of the REGISTER, bytes is how many bytes are being read/written, 
//	data is a pointer to a buffer for reading/writing
// OUTPUT:
//	A I2C write tx will be sent across the SERCOM bus
//------------------------------------------------------
void initWrite(uint8_t addr, size_t bytes,volatile unsigned char *data){
	size_t count = 0;	
	initTx(addr,bytes,data,WRITE);
	
	while( count < bytes){
		
		while((SERCOM2_REGS->I2CM.SERCOM_INTFLAG) == 0U){
		
			//wait for sync of SYNCBUSY.SYSOP to ensure we are not doing anything on the bus

		}//while
		 
		SERCOM2_REGS->I2CM.SERCOM_DATA = data[count];
		count++;

	}//while	


	//Don't need to NACK the gyro according to its data sheet just send a stop
	SERCOM2_REGS->I2CM.SERCOM_CTRLB |= SERCOM_I2CM_CTRLB_CMD(3UL);
		

	while((SERCOM2_REGS->I2CM.SERCOM_SYNCBUSY) != 0U){
		

	}//while
	count = 0;
}//initWrite
