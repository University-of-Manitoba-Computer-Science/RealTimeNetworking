#include "button.h"

#include <sam.h>

void buttonInit()
{
    // switch input on PA15, processed as an external interrupt
    PORT_REGS->GROUP[0].PORT_DIRCLR      = PORT_PA15;
    PORT_REGS->GROUP[0].PORT_PINCFG[15] |= PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[0].PORT_PMUX[7]    |= PORT_PMUX_PMUXO_A;

    PORT_REGS->GROUP[0].PORT_OUTSET      = PORT_PA15;
    PORT_REGS->GROUP[0].PORT_PINCFG[15] |= PORT_PINCFG_PULLEN_Msk;

    // have to enable the interrupt line in the system level REG
    NVIC_EnableIRQ(EIC_EXTINT_15_IRQn);

    MCLK_REGS->MCLK_APBAMASK |= MCLK_APBAMASK_EIC_Msk;

    // the button is noisy so we keep it slow and lots of samples for filtering
    // -- not great but it does the job (usually)
    EIC_REGS->EIC_CONFIG[1] |=
        ((uint32_t) (EXTINT15_MASK) << 16) | EIC_CONFIG_SENSE7_RISE;
    EIC_REGS->EIC_DEBOUNCEN  |= EXTINT15_MASK;
    EIC_REGS->EIC_DPRESCALER |= EIC_DPRESCALER_TICKON_Msk |
                                EIC_DPRESCALER_STATES1_Msk |
                                EIC_DPRESCALER_PRESCALER1_DIV64;
    EIC_REGS->EIC_INTENSET |= EXTINT15_MASK;
    EIC_REGS->EIC_CTRLA    |= EIC_CTRLA_CKSEL_CLK_ULP32K | EIC_CTRLA_ENABLE_Msk;
}
