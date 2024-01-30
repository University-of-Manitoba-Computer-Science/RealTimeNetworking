#include <assert.h>
#include <sam.h>

#include "src/dcc_stdio.h"

// setup our heartbeat to be 1ms: we overflow at 1ms intervals with a 120MHz
// clock uses the SysTicks unit so that we get reliable debugging (timer stops
// on breakpoints)
#define MS_TICKS 120000UL

// number of millisecond between LED flashes
#define LED_FLASH_MS 250UL

#define EXTINT15_MASK 0x8000

// NOTE: this overflows every ~50 days, so I'm not going to care here...
volatile uint32_t msCount = 0;

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

void canInit()
{
    // setup clock
    GCLK_REGS->GCLK_PCHCTRL[27] = GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[27] & GCLK_PCHCTRL_CHEN_Msk) !=
           GCLK_PCHCTRL_CHEN_Msk)
        ;
    MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_CAN0_Msk;

    // Configuration
    CAN0_REGS->CAN_CCCR |= CAN_CCCR_INIT_Msk;
    CAN0_REGS->CAN_CCCR |= CAN_CCCR_CCE_Msk | CAN_CCCR_TEST_Msk;

    CAN0_REGS->CAN_TEST = CAN_TEST_LBCK_Msk; // Set loopback mode
    // TODO set filter config?
    // TODO I think Rx Buffer configuration is fine by default?
    CAN0_REGS->CAN_TXBC = CAN_TXBC_TFQS(32); // TODO I think we want this?

    // unset CCCR.INIT once synchronized
    while ((CAN0_REGS->CAN_CCCR & CAN_CCCR_INIT_Msk) != CAN_CCCR_INIT_Msk)
        ;
    CAN0_REGS->CAN_CCCR &= ~CAN_CCCR_INIT_Msk;
}

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

// Fires every 1ms
void SysTick_Handler()
{
    msCount++;
}

// ISR for  external interrupt 15, add processing code as required...
void EIC_EXTINT_15_Handler()
{
    // clear the interrupt! and go to the next operating mode
    EIC_REGS->EIC_INTFLAG           |= EXTINT15_MASK;
    PORT_REGS->GROUP[0].PORT_OUTTGL  = PORT_PA14;
}

uint8_t i = 0;

void test()
{
    dbg_write_u8(&i, 1);
    i++;
    dbg_write_str("\n");
}

int main(void)
{
#ifndef NDEBUG
    for (int i = 0; i < 100000; i++)
        ;
#endif

    // NOTE: the silkscreen on the curiosity board is WRONG! it's PB4 and PB5
    // NOT PA4 and PA5

    // see the header files within include/component for register definitions,
    // which align with the data sheet for the processor e.g. port.h contains
    // the masks and definitions for manipulating gpio

    // LED output
    PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA14;
    PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA14;

    // enable cache
    // tradeoff: +: really helps with repeated code/data (like when doing
    // animations in a game)
    //           -: results in non-deterministic run-times
    //           +: there *is* a way to lock lines of cache to keep hard
    //           deadline code/data pinned in the cache
    if ((CMCC_REGS->CMCC_SR & CMCC_SR_CSTS_Msk) == 0)
        CMCC_REGS->CMCC_CTRL = CMCC_CTRL_CEN_Msk;

    // sleep to idle (wake on interrupts)
    PM_REGS->PM_SLEEPCFG |= PM_SLEEPCFG_SLEEPMODE_IDLE;

    heartInit();
    buttonInit();
    canInit();

    // we want interrupts!
    __enable_irq();

    // sleep until we have an interrupt
    while (1) {
        __WFI();
        // TODO: make proper task scheduling!
        if ((msCount % LED_FLASH_MS) == 0) {
            // PORT_REGS->GROUP[0].PORT_OUTTGL = PORT_PA14;
            test();
        }
    }
    return 0;
}
