#include "heart.h"

// setup our heartbeat to be 1ms: we overflow at 1ms intervals with a 120MHz
// clock uses the SysTicks unit so that we get reliable debugging (timer stops
// on breakpoints)
#define MS_TICKS 120000UL

volatile uint32_t msCount = 0;

uint32_t get_ticks()
{
    return msCount;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = msCount;
    while (msCount - start < ms)
    {
        __WFI();
    }
}

void heartInit()
{
    // setup the main clock/CPU clock to run at 120MHz

    // set generator 2 to use DFLL48M as source; with a divider of 48 that gives
    // us 1MHz
    GCLK_REGS->GCLK_GENCTRL[2] =
        GCLK_GENCTRL_DIV(48) | GCLK_GENCTRL_SRC_DFLL | GCLK_GENCTRL_GENEN_Msk;
    while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL_GCLK2) ==
           GCLK_SYNCBUSY_GENCTRL_GCLK2)
        ; /* Wait for synchronization */

    // set DPLL0 peripheral to use generator 2 as its clock
    // see page 156 of data sheet for GCLK array offsets
    GCLK_REGS->GCLK_PCHCTRL[1] = GCLK_PCHCTRL_GEN_GCLK2 | GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[1] & GCLK_PCHCTRL_CHEN_Msk) !=
           GCLK_PCHCTRL_CHEN_Msk)
        ; /* Wait for synchronization */

    // DPLL in integer mode: multiply generator clk by 120, giving us 120MHz
    OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLCTRLB = OSCCTRL_DPLLCTRLB_FILTER(0) |
                                              OSCCTRL_DPLLCTRLB_LTIME(0) |
                                              OSCCTRL_DPLLCTRLB_REFCLK(0);
    OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLRATIO =
        OSCCTRL_DPLLRATIO_LDRFRAC(0) | OSCCTRL_DPLLRATIO_LDR(119);
    while ((OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLSYNCBUSY &
            OSCCTRL_DPLLSYNCBUSY_DPLLRATIO_Msk) ==
           OSCCTRL_DPLLSYNCBUSY_DPLLRATIO_Msk)
        ; /* Wait for synchronization */

    // Enable DPLL0
    OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLCTRLA = OSCCTRL_DPLLCTRLA_ENABLE_Msk;
    while ((OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLSYNCBUSY &
            OSCCTRL_DPLLSYNCBUSY_ENABLE_Msk) == OSCCTRL_DPLLSYNCBUSY_ENABLE_Msk)
        ; /* Wait for synchronization */

    while ((OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLSTATUS &
            (OSCCTRL_DPLLSTATUS_LOCK_Msk | OSCCTRL_DPLLSTATUS_CLKRDY_Msk)) !=
           (OSCCTRL_DPLLSTATUS_LOCK_Msk | OSCCTRL_DPLLSTATUS_CLKRDY_Msk))
        ; /* Wait for ready state */

    // define our main generic clock, which drives everything, to be 120MHz from
    // the PLL
    MCLK_REGS->MCLK_CPUDIV = MCLK_CPUDIV_DIV_DIV1;
    while ((MCLK_REGS->MCLK_INTFLAG & MCLK_INTFLAG_CKRDY_Msk) !=
           MCLK_INTFLAG_CKRDY_Msk)
        ; /* Wait for main clock to be ready */

    GCLK_REGS->GCLK_GENCTRL[0] = GCLK_GENCTRL_DIVSEL_DIV1 |
                                 GCLK_GENCTRL_SRC_DPLL0 |
                                 GCLK_GENCTRL_GENEN_Msk;
    while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL_GCLK0) ==
           GCLK_SYNCBUSY_GENCTRL_GCLK0)
        ; /* Wait for synchronization */

    // have to enable the interrupt line in the system level REG
    NVIC_EnableIRQ(SysTick_IRQn);

    SysTick_Config(MS_TICKS);
}

void SysTick_Handler()
{
    msCount++;
}