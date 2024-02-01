#include "sam.h"

void canbus_init()
{
    // enable peripheral clock for CAN0 (27) [155]
    GCLK_REGS->GCLK_PCHCTRL[CAN0_GCLK_ID] = GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[CAN0_GCLK_ID] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk)
        ; /* Wait for synchronization */

    // enable AHB clock for CAN0 (27) [pg. 127]
    MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_CAN0_Msk;

    // configure CAN0
    CAN0_REGS->CAN_CCCR = CAN_CCCR_INIT_Msk;
    while ((CAN0_REGS->CAN_CCCR & CAN_CCCR_INIT_Msk) != CAN_CCCR_INIT_Msk)
        ; /* Wait for synchronization */

    // enable configuration change [pg. 1154]
    CAN0_REGS->CAN_CCCR |= CAN_CCCR_CCE_Msk;
}