#include "can.h"

//HEAVY influence from the following repo
//https://github.com/majbthrd/SAMC21demoCAN


// We will use PA22 AND PA23 as are CAN TX and CAN RX respectively
#define CAN_CCCR_INIT_MODE_Msk (CAN_CCCR_INIT_Msk & CAN_CCCR_CCE_Msk) // This is so we can set INIT nad CCE at once
void clkCAN(){

    GCLK_REGS->GCLK_PCHCTRL[CAN0_GCLK_ID] |= GCLK_PCHCTRL_CHEN_Msk;
	while((GCLK_REGS->GCLK_PCHCTRL[CAN0_GCLK_ID] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk){
		//wait for sync
	}//while
	
    MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_CAN0_Msk;

}//clkCAN

void initCAN(uint32_t *ram){

	//set the init bit as per 39.6.2.1 to start init
	CAN0_REGS->CAN_CCCR |= CAN_CCCR_INIT_Msk; //set CCCR.INIT to 1 so we can set CCE to 1 then being setting up CAN controller
	while((CAN0_REGS->CAN_CCCR & CAN_CCCR_INIT_Msk)!= CAN_CCCR_INIT_Msk){
		//wait for init to be set before we set CCE

	} 
	CAN0_REGS->CAN_CCCR |= CAN_CCCR_CCE_Msk; //set CCE so we can un-write protect the rest of the registers
	while((CAN0_REGS->CAN_CCCR & (CAN_CCCR_INIT_Msk | CAN_CCCR_CCE_Msk))!= (CAN_CCCR_INIT_Msk |CAN_CCCR_CCE_Msk)){
		//Make sure we have all our init and cce bit sets so we can config

	} 

	//SAMC21 demo sets global filter config

	//SAMC21 demo sets interrupts

	//SAMC21 demo sets bit timing
	//they get the generated clock here via pmc_get_gck_clock
	//page 39.8.8 is where the nominal bit timing prescaler
	CAN0_REGS->CAN_NBTP = CAN_NBTP_NTSEG1(QUANTA_BEFORE-2) | CAN_NBTP_NTSEG2(QUANTA_AFTER-1) | CAN_NBTP_NSJW(QUANTA_SYNC-1);

	//SAMC21 demo sets RAM starting addresses and element count here
	CAN0_REGS->CAN_SIDFC = CAN_SIDFC_FLSSA(ram) | CAN_SIDFC_LSS(MSG_LIST_SIZE);

	//turn on loopback mode for testing
	CAN0_REGS->CAN_TEST |= CAN_TEST_LBCK_Msk;



	
	//CAN bus messages are stopped CAN_TX is high/recessive
	//CCCR.CCE and CCCR.INIT need to be set to 1 to change register config
	//CCCR.CCE should be set/reset when CCCR.INIT = 1 so say 39.6.2.1


	//We finish but resetting CCCR.INIT as per 39.6.2.1
	CAN0_REGS->CAN_CCCR &= ~CAN_CCCR_INIT_Msk;
	
}


void CAN0Init(){

//Initiate CAN0 TX port
PORT_REGS->GROUP[0].PORT_PINCFG[22] |= PORT_PINCFG_PMUXEN_Msk;
PORT_REGS->GROUP[0].PORT_PMUX[11] |= PORT_PMUX_PMUXE_I;    

//Initiate CAN0 RX port
PORT_REGS->GROUP[0].PORT_PINCFG[23] |= PORT_PINCFG_PMUXEN_Msk;
PORT_REGS->GROUP[0].PORT_PMUX[11] |= PORT_PMUX_PMUXO_I;    


}//initCAN