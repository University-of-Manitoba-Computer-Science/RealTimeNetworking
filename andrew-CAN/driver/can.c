#include "can.h"
// We will use PA22 AND PA23 as are CAN TX and CAN RX respectively
void clkCAN(){

    GCLK_REGS->GCLK_PCHCTRL[CAN0_GCLK_ID] |= GCLK_PCHCTRL_CHEN_Msk;
	while((GCLK_REGS->GCLK_PCHCTRL[CAN0_GCLK_ID] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk){
		//wait for sync
	}//while
	
    MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_CAN0_Msk;

}//clkCAN

void initCAN(){

//set the init bit as per 39.6.2.1 to start init
CAN0_REGS->CAN_CCCR = CAN_CCCR_INIT_Msk;
//CAN bus messages are stopped CAN_TX is high/recessive
//CCCR.CCE and CCCR.INIT need to be set to 1 to change register config
//CCCR.CCE should be set/reset when CCCR.INIT = 1 so say 39.6.2.1


//We finish but resetting CCCR.INIT as per 39.6.2.1
CAN0_REGS->CAN_CCCR = CAN_CCCR_INIT_Msk;

}


void CAN0Init(){

//Initiate CAN0 TX port
PORT_REGS->GROUP[0].PORT_PINCFG[22] |= PORT_PINCFG_PMUXEN_Msk;
PORT_REGS->GROUP[0].PORT_PMUX[11] |= PORT_PMUX_PMUXE_I;    

//Initiate CAN0 RX port
PORT_REGS->GROUP[0].PORT_PINCFG[23] |= PORT_PINCFG_PMUXEN_Msk;
PORT_REGS->GROUP[0].PORT_PMUX[11] |= PORT_PMUX_PMUXO_I;    


}//initCAN