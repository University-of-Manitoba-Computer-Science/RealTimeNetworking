#include "can.h"

#include <sam.h>

void canInit()
{
    // setup TX port
    PORT_REGS->GROUP[0].PORT_DIRSET      = PORT_PA22;
    PORT_REGS->GROUP[0].PORT_PINCFG[22] |= PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[0].PORT_PMUX[11]   |= PORT_PMUX_PMUXE_I;

    // setup RX port
    PORT_REGS->GROUP[0].PORT_DIRCLR      = PORT_PA23;
    PORT_REGS->GROUP[0].PORT_PINCFG[23] |= PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[0].PORT_PMUX[11]   |= PORT_PMUX_PMUXO_I;

    // setup clock
    GCLK_REGS->GCLK_PCHCTRL[27] = GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[27] & GCLK_PCHCTRL_CHEN_Msk) !=
           GCLK_PCHCTRL_CHEN_Msk)
        ;
    MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_CAN0_Msk;

    // Configuration
    CAN0_REGS->CAN_CCCR |= CAN_CCCR_INIT_Msk; // start initialization
    CAN0_REGS->CAN_CCCR |= CAN_CCCR_CCE_Msk;  // enable configuration

    // TODO remove once CAN driver is working over loopback
    CAN0_REGS->CAN_CCCR |= CAN_CCCR_TEST_Msk; // Enable test mode
    CAN0_REGS->CAN_TEST  = CAN_TEST_LBCK_Msk; // Enable loopback mode

    // Reject non-matching frames
    // TODO I don't know if we need this, but testing some filtering seems like
    // a good idea
    CAN0_REGS->CAN_GFC |= CAN_GFC_ANFS(2) | CAN_GFC_ANFE(2);

    // TODO do we need to set bit rate? Using defaults for now

    // Setting up message RAM
    CAN0_REGS->CAN_SIDFC = CAN_SIDFC_FLSSA(0) | CAN_SIDFC_LSS(128);
    CAN0_REGS->CAN_XIDFC = CAN_XIDFC_FLESA(128) | CAN_XIDFC_LSE(64);
    CAN0_REGS->CAN_RXF0C =
        CAN_RXF0C_F0OM_Msk | CAN_RXF0C_F0S(0) | CAN_RXF0C_F0SA(256);
    CAN0_REGS->CAN_RXF0C =
        CAN_RXF0C_F0OM_Msk | CAN_RXF0C_F0S(0) | CAN_RXF0C_F0SA(1408);
    CAN0_REGS->CAN_TXBC = CAN_TXBC_TFQM_Msk | CAN_TXBC_TFQS(32) |
                          CAN_TXBC_NDTB(32) | CAN_TXBC_TBSA(3776);

    // unset CCCR.INIT once synchronized
    while ((CAN0_REGS->CAN_CCCR & CAN_CCCR_INIT_Msk) != CAN_CCCR_INIT_Msk)
        ;
    CAN0_REGS->CAN_CCCR &= ~CAN_CCCR_INIT_Msk;
}
