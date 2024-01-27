#include "sam.h"
#include <assert.h>

// overflow @ 1ms w/ 120MHz clock
#define MS_TICKS 120000UL
#define LED_FLASH_MS 250UL
#define EXTINT15_MASK 0x8000

volatile uint32_t msCount = 0;

void heartInit()
{
  // set generator 2 to use DFLL48M as source; with a divider of 48 that gives us 1MHz
  GCLK_REGS->GCLK_GENCTRL[2] = GCLK_GENCTRL_DIV(48) | GCLK_GENCTRL_SRC_DFLL | GCLK_GENCTRL_GENEN_Msk;
  while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL_GCLK2) == GCLK_SYNCBUSY_GENCTRL_GCLK2)
    ; /* Wait for synchronization */

  // set DPLL0 peripheral to use generator 2 as its clock (pg. 156 for the GCLK array offsets)
  GCLK_REGS->GCLK_PCHCTRL[1] = GCLK_PCHCTRL_GEN_GCLK2 | GCLK_PCHCTRL_CHEN_Msk;
  while ((GCLK_REGS->GCLK_PCHCTRL[1] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk)
    ; /* Wait for synchronization */

  // DPLL in integer mode: multiply generator clk by 120, giving us 120MHz
  OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLCTRLB = OSCCTRL_DPLLCTRLB_FILTER(0) | OSCCTRL_DPLLCTRLB_LTIME(0) | OSCCTRL_DPLLCTRLB_REFCLK(0);
  OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLRATIO = OSCCTRL_DPLLRATIO_LDRFRAC(0) | OSCCTRL_DPLLRATIO_LDR(119);
  while ((OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLSYNCBUSY & OSCCTRL_DPLLSYNCBUSY_DPLLRATIO_Msk) == OSCCTRL_DPLLSYNCBUSY_DPLLRATIO_Msk)
    ; /* Wait for synchronization */

  // Enable DPLL0
  OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLCTRLA = OSCCTRL_DPLLCTRLA_ENABLE_Msk;
  while ((OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLSYNCBUSY & OSCCTRL_DPLLSYNCBUSY_ENABLE_Msk) == OSCCTRL_DPLLSYNCBUSY_ENABLE_Msk)
    ; /* Wait for synchronization */

  while ((OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLSTATUS & (OSCCTRL_DPLLSTATUS_LOCK_Msk | OSCCTRL_DPLLSTATUS_CLKRDY_Msk)) !=
         (OSCCTRL_DPLLSTATUS_LOCK_Msk | OSCCTRL_DPLLSTATUS_CLKRDY_Msk))
    ; /* Wait for ready state */

  // define our main generic clock, which drives everything, to be 120MHz from the PLL
  MCLK_REGS->MCLK_CPUDIV = MCLK_CPUDIV_DIV_DIV1;
  while ((MCLK_REGS->MCLK_INTFLAG & MCLK_INTFLAG_CKRDY_Msk) != MCLK_INTFLAG_CKRDY_Msk)
    ; /* Wait for main clock to be ready */

  GCLK_REGS->GCLK_GENCTRL[0] = GCLK_GENCTRL_DIVSEL_DIV1 | GCLK_GENCTRL_SRC_DPLL0 | GCLK_GENCTRL_GENEN_Msk;
  while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL_GCLK0) == GCLK_SYNCBUSY_GENCTRL_GCLK0)
    ; /* Wait for synchronization */

  // have to enable the interrupt line in the system level REG
  NVIC_EnableIRQ(SysTick_IRQn);
  SysTick_Config(MS_TICKS);
}

void buttonInit()
{
  // switch input on PA15, processed as an external interrupt
  PORT_REGS->GROUP[0].PORT_DIRCLR = PORT_PA15;
  PORT_REGS->GROUP[0].PORT_PINCFG[15] |= PORT_PINCFG_PMUXEN_Msk;
  PORT_REGS->GROUP[0].PORT_PMUX[7] |= PORT_PMUX_PMUXO_A;

  PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA15;
  PORT_REGS->GROUP[0].PORT_PINCFG[15] |= PORT_PINCFG_PULLEN_Msk;

  // have to enable the interrupt line in the system level REG
  NVIC_EnableIRQ(EIC_EXTINT_15_IRQn);

  MCLK_REGS->MCLK_APBAMASK |= MCLK_APBAMASK_EIC_Msk;

  // the button is noisy so we keep it slow and lots of samples for filtering -- not great but it does the job (usually)
  EIC_REGS->EIC_CONFIG[1] |= ((uint32_t)(EXTINT15_MASK) << 16) | EIC_CONFIG_SENSE7_RISE;
  EIC_REGS->EIC_DEBOUNCEN |= EXTINT15_MASK;
  EIC_REGS->EIC_DPRESCALER |= EIC_DPRESCALER_TICKON_Msk | EIC_DPRESCALER_STATES1_Msk | EIC_DPRESCALER_PRESCALER1_DIV64;
  EIC_REGS->EIC_INTENSET |= EXTINT15_MASK;
  EIC_REGS->EIC_CTRLA |= EIC_CTRLA_CKSEL_CLK_ULP32K | EIC_CTRLA_ENABLE_Msk;
}

// fires every 1ms
void SysTick_Handler()
{
  msCount++;
}

// button press handler
void EIC_EXTINT_15_Handler()
{
  EIC_REGS->EIC_INTFLAG |= EXTINT15_MASK;
}

int main(void)
{
  // setup led output on PA14
  PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA14;
  PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA14;

  // enable cache (see main branch for tradeoffs)
  if ((CMCC_REGS->CMCC_SR & CMCC_SR_CSTS_Msk) == 0)
    CMCC_REGS->CMCC_CTRL = CMCC_CTRL_CEN_Msk;

  // sleep to idle (wake on interrupts)
  PM_REGS->PM_SLEEPCFG |= PM_SLEEPCFG_SLEEPMODE_IDLE;

  heartInit();
  buttonInit();

  // we want interrupts!
  __enable_irq();

  // sleep until we have an interrupt
  while (1)
  {
    __WFI();

    if ((msCount % LED_FLASH_MS) == 0)
    {
      // PORT_REGS->GROUP[0].PORT_OUTTGL = PORT_PA14;
    }
  }
  return 0;
}
