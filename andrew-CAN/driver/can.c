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

void initCAN(uint32_t *ram, uint32_t *tx, uint32_t *rx, uint32_t *event, uint32_t *rxbuff){

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
	CAN0_REGS->CAN_NBTP = (CAN_NBTP_NTSEG1(QUANTA_BEFORE-2) | CAN_NBTP_NTSEG2(QUANTA_AFTER-1) | CAN_NBTP_NSJW(QUANTA_SYNC-1));

	//SAMC21 demo sets message RAM starting addresses and element count here
	CAN0_REGS->CAN_SIDFC = (CAN_SIDFC_FLSSA(ram) | CAN_SIDFC_LSS(MEM_SIZE)); //sets memorya ddr and size for message filter
	
	//SAMC21 demo sets Rx RAM starting addresses and element count here
	CAN0_REGS->CAN_RXF0C = (CAN_RXF0C_F0SA(rx) | CAN_RXF0C_F0S(MEM_SIZE) | CAN_RXF0C_F0OM(0) | CAN_RXF0C_F0OM(0)); //sets Rx fifo  RAM starting address and size turns off watermarks and then turns on blocking mode for the fifo operation

	//SAMC21 demo sets Tx RAM starting addresses and element count here
	CAN0_REGS->CAN_TXBC = (CAN_TXBC_TBSA(tx) | CAN_TXBC_NDTB(MSG_LIST_SIZE) | CAN_TXBC_TFQM(MEM_SIZE) | CAN_TXBC_TFQM(0)); //set Tx fifo RAM starting address and size of array then size of fifo, then we turn on FIFO operation 

	//SAMC21 demo sets Tx Event fifo
	CAN0_REGS->CAN_TXEFC = (CAN_TXEFC_EFSA(event) | CAN_TXEFC_EFS(MEM_SIZE) | CAN_TXEFC_EFWM(0)); //Set our tx event fifo addr and size then turn off watermark

	//SAMC21 demo sets rx buffer 

	CAN0_REGS->CAN_RXBC = (CAN_RXBC_RBSA(rxbuff)); //as stated in the demo the hardware doesn't care about how many rx bufferes there are 

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

//rx and filter
void setFifoFilter(int fifo, uint8_t filter, uint32_t id, uint32_t mask){

	if(fifo == 0 || fifo == 1){
		
	}

}
//checks if RX buffer has a item at the specified index
void getCanRxBuffData(uint8_t index, CAN_MSG *msg, uint32_t *ramAddr){
	uint32_t *rxBuf = 0; 
	uint32_t rxWord;
	uint8_t length;

	//If we are out of range set the msg to empty
	if(index >= MSG_LIST_SIZE){
		msg->id = 0; 
		msg->time = 0; 
		msg->len = 0;
		msg->dataLen = 0;
	}
	else {
		//TODO: Finish getting message from ram
		rxBuf = ramAddr + (index * (RX_HEADER_SZ + MEM_SIZE/4));
		rxWord = *rxBuf++; // we sshould now have the word of the rxBuffData
		// dont want the extended
		msg->id = (rxWord & CAN_RXF0E_0_ID_Msk); //mask our word with the ID Msk -> probably wrong
		msg->len = 0;
		msg->time = rxWord; //wrong
		
	}
}

bool hasRxBuffData(uint8_t index){
	bool out = (CAN0_REGS->CAN_NDAT1 &(1 << index)) ? true : false;
	if (index > 32 && index < 64){
		out = (CAN0_REGS->CAN_NDAT2 &(1 << index -32)) ? true : false; 
	}
	else if(index < 0 || index > 64){

		out = false; 
	}
	return out;


}

//tx 3
void sendCanTXbuffer(uint8_t index){
	

}
void enqueueCanTxMsg(uint32_t id, uint8_t length, const uint8_t *data){


}
