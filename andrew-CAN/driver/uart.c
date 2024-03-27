#include "uart.h"

//reference
//https://microchip-mplab-harmony.github.io/reference_apps/apps/sam_e51_cnano/same51n_getting_started/readme.html


void clkUART(){

    //I want the UART on an 8mhz clock for maybe uneducated reasons (I want it to get 1mbps transmit speed)
    GCLK_REGS->GCLK_GENCTRL[4] = GCLK_GENCTRL_SRC_DPLL0 | GCLK_GENCTRL_GENEN_Msk;
	GCLK_REGS->GCLK_PCHCTRL[SERCOM0_GCLK_ID_CORE] = GCLK_PCHCTRL_GEN_GCLK4 | GCLK_PCHCTRL_CHEN_Msk;
	while((GCLK_REGS->GCLK_PCHCTRL[SERCOM0_GCLK_ID_CORE] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk){
		//wait for sync
	}//while	

    MCLK_REGS->MCLK_APBAMASK |= MCLK_APBAMASK_SERCOM0_Msk;

}
void initUART(){

    //The following comes from section 34.6.2.1
    //disable the USART sercom
    SERCOM0_REGS->USART_INT.SERCOM_CTRLA = SERCOM_USART_INT_CTRLA_ENABLE(0);
    

    //todo: I have changed how TXP0 works along with RXP0 to select PAD1 for RX as per the spec, but I need to make sure the TXP0 mode change from 0x0 to 0x2 works
    //The following sets up CTRLA as follows
    //selects async mode, with internal clock, RX (PAD1) and TX (PAD0) on SERCOM0 with internal clock, sets to MSB mode
    //Not sure if we want TXP0 as mode 0x0 (sercom pad 0 for rx but pad 1 gets used for exck) or mode 0x2 where we dont use pad 1 for anything else. 
    SERCOM0_REGS->USART_INT.SERCOM_CTRLA = SERCOM_USART_INT_CTRLA_MODE_USART_INT_CLK |  SERCOM_USART_INT_CTRLA_RXPO(RX_MODE) | SERCOM_USART_INT_CTRLA_TXPO(TX_MODE) | SERCOM_USART_INT_CTRLA_DORD_Msk | SERCOM_USART_INT_CTRLA_FORM(0X0UL) | SERCOM_USART_INT_CTRLA_IBON_Msk | SERCOM_USART_INT_CTRLA_SAMPR(0x0);

    //The following sets up CTRLB as follows
    SERCOM0_REGS->USART_INT.SERCOM_CTRLB = SERCOM_USART_INT_CTRLB_CHSIZE_8_BIT | SERCOM_USART_INT_CTRLB_SBMODE_1_BIT;
    while((SERCOM0_REGS->USART_INT.SERCOM_SYNCBUSY & SERCOM_USART_INT_SYNCBUSY_CTRLB_Msk) != 0){
        //Wait for CTRLB enable
    }

    //Since we are using internal clock we can set desired baud rate with our 8Mhz clock (gclk4)
    SERCOM0_REGS->USART_INT.SERCOM_BAUD = SERCOM_USART_INT_BAUD_BAUD(UART_BUAD);
    //enable transmit and receive
    

    //Enable sercom
    SERCOM0_REGS->USART_INT.SERCOM_CTRLA |= SERCOM_USART_INT_CTRLA_ENABLE_Msk;
    while((SERCOM0_REGS->USART_INT.SERCOM_SYNCBUSY & SERCOM_USART_INT_SYNCBUSY_ENABLE_Msk) != 0){
        //Wait for enable
    }
}
void portUART(){
    
    //USART sercom0 on PORY_PA8
    PORT_REGS->GROUP[0].PORT_PINCFG[8] |= PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[0].PORT_PMUX[4] |= PORT_PMUX_PMUXE_C;

 //USART sercom0 on PORY_PA9
    PORT_REGS->GROUP[0].PORT_PINCFG[9] |= PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[0].PORT_PMUX[4] |= PORT_PMUX_PMUXO_C;

}

void txUART(uint8_t data){
    SERCOM0_REGS->USART_INT.SERCOM_CTRLB |= SERCOM_USART_INT_CTRLB_TXEN_Msk;
    while((SERCOM0_REGS->USART_INT.SERCOM_SYNCBUSY & SERCOM_USART_INT_SYNCBUSY_CTRLB_Msk) != 0){
        //Wait for CTRLB to sync
    }

    while((SERCOM0_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk) == 0U ){
        //WAIT for clear int flag

    }

    SERCOM0_REGS->USART_INT.SERCOM_DATA = (uint16_t)data;

    SERCOM0_REGS->USART_INT.SERCOM_CTRLB &= ~(SERCOM_USART_INT_CTRLB_TXEN_Msk);
    while((SERCOM0_REGS->USART_INT.SERCOM_SYNCBUSY & SERCOM_USART_INT_SYNCBUSY_CTRLB_Msk) != 0){
        //Wait for CTRLB to sync
    }

}

uint8_t rxUART(){

    return SERCOM0_REGS->USART_INT.SERCOM_DATA;

}