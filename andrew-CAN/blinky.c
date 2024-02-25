#include "sam.h"
#include "dcc_stdio.h"
#include "can.h"
#include "i2c.h"
#include "button.h"
#include <assert.h>


#define DEBUG_WAIT 10000000UL
// setup our heartbeat to be 1ms: we overflow at 1ms intervals with a 120MHz clock
// uses the SysTicks unit so that we get reliable debugging (timer stops on breakpoints)
#define MS_TICKS 120000UL

// number of millisecond between LED flashes
#define LED_FLASH_MS 250UL
#define GYRO_CHECK_MS 510UL
void heartInit();

// NOTE: this overflows every ~50 days, so I'm not going to care here...
volatile uint32_t msCount = 0;


void initAllPorts(){

    // LED output
  PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA14;
  PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA14;

  port15Init(); //init button ports
  sercom2Init(); //init sercom2 -> i2c ports
  CAN0Init(); //init CAN0 ports

}//initAllPorts

void initAllClks(){

  clkButton();
  clkI2C();
  clkCAN();

}//initAllClks

void initAll(){

  heartInit();
  initAllClks();
  initAllPorts();
  initI2C();
  initButton();
  initCAN();
  accelOnlyMode();
}


// provides debugger only assertion support
// we are providing the routine called by the standard assert marcro in assert.h
// if NDEBUG is defined (for release code) the macro does *not* call this function, it's a no-op
// NOTE: the standard definition has a no_return attribute which we are *not* doing; we breakpoint but can continue execution
//       we'll live with this one warning...
void __assert_func(const char *fileName, int lineNum, const char *caller, const char *expression){
  // don't really need any of this printing as the debugger shows all of this as part of the SIGTRAP information
  dbg_write_str("Assertion '");
  dbg_write_str(expression);
  dbg_write_str("' failed in ");
  dbg_write_str(fileName);
  dbg_write_char(':');
  dbg_write_str(caller);
  dbg_write_char('(');
  // this could be more robust; just printing the hex value is easier but less useful
  dbg_write_char(lineNum / 1000 + '0');
  dbg_write_char((lineNum / 100) % 10 + '0');
  dbg_write_char((lineNum / 10) % 10 + '0');
  dbg_write_char(lineNum % 10 + '0');
  dbg_write_char(')');

  // for debugging purposes *this* is all the code we really need
  // don't breakpoint if we're not running via a debugger, otherwise we stop 'forever'
  if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) == CoreDebug_DHCSR_C_DEBUGEN_Msk)
    __BKPT(0);
}

void heartInit(){
  // setup the main clock/CPU clock to run at 120MHz

  // set generator 2 to use DFLL48M as source; with a divider of 48 that gives us 1MHz
  GCLK_REGS->GCLK_GENCTRL[2] = GCLK_GENCTRL_DIV(48) | GCLK_GENCTRL_SRC_DFLL | GCLK_GENCTRL_GENEN_Msk;
  while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL_GCLK2) == GCLK_SYNCBUSY_GENCTRL_GCLK2)
    ; /* Wait for synchronization */

  // set DPLL0 peripheral to use generator 2 as its clock
  // see page 156 of data sheet for GCLK array offsets
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


// Fires every 1ms
void SysTick_Handler(){
  msCount++;
}

// ISR for  external interrupt 15, add processing code as required...
void EIC_EXTINT_15_Handler(){
  PORT_REGS->GROUP[0].PORT_OUTTGL = PORT_PA14;
  // clear the interrupt! and go to the next operating mode
  EIC_REGS->EIC_INTFLAG |= EXTINT15_MASK;
}

void test(){
  dbg_write_str("beforee");
  PORT_REGS->GROUP[0].PORT_OUTTGL = PORT_PA14;
  dbg_write_str("afterr");
}

int main(void){



#ifndef NDEBUG
  for (int i = 0; i < DEBUG_WAIT; i++);
#endif

  // NOTE: the silkscreen on the curiosity board is WRONG! it's PB4 and PB5 NOT PA4 and PA5

  // see the header files within include/component for register definitions, which align with the data sheet for the processor
  // e.g. port.h contains the masks and definitions for manipulating gpio



  // enable cache
  // tradeoff: +: really helps with repeated code/data (like when doing animations in a game)
  //           -: results in non-deterministic run-times
  //           +: there *is* a way to lock lines of cache to keep hard deadline code/data pinned in the cache
  if ((CMCC_REGS->CMCC_SR & CMCC_SR_CSTS_Msk) == 0)
    CMCC_REGS->CMCC_CTRL = CMCC_CTRL_CEN_Msk;

  // sleep to idle (wake on interrupts)
  PM_REGS->PM_SLEEPCFG |= PM_SLEEPCFG_SLEEPMODE_IDLE;

  initAll();

  // we want interrupts!
  __enable_irq();

  // some example logging calls
#ifndef NDEBUG
  dbg_write_str("~~~DEBUG ENABLED~~~\n");

/*
  dbg_write_char('t');
  dbg_write_char('e');
  dbg_write_char('s');
  dbg_write_char('t');
  dbg_write_char('\n');

  unsigned long test_u32 = 0x01234567;
  dbg_write_u32(&test_u32, 1);

  static const unsigned short test_u16[] = {0x0123, 0x4567, 0x89AB, 0xCDEF, 0x0123, 0x4567, 0x89AB, 0xCDEF};
  dbg_write_u16(test_u16, 8);

  static const unsigned char test_u8[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0XDD, 0xEE, 0xFF};
  dbg_write_u8(test_u8, 16);
  */
#endif
  PORT_REGS->GROUP[0].PORT_OUTTGL = PORT_PA14;
  // sleep until we have an interrupt
  while (1){
    __WFI();
    /* uncomment the line below to trace every time this code is reached
     * WARNING: making this call induces a SIGNIFICANT delay, use sparingly and wisely (if at all)
     *          calling it every time you wake is **not** wise
     * to show how often the trace point was hit use the following debugger command:
     * monitor trace point
     */
    //    dbg_trace_point(0);

    // demo an assertion firing into the debugger
    // assert(msCount != 250);

    // TODO: make proper task scheduling!
    if ((msCount % LED_FLASH_MS) == 0){
      PORT_REGS->GROUP[0].PORT_OUTTGL = PORT_PA14;
      #ifndef NDEBUG
        dbg_write_u32(&msCount, 1);
      #endif
     
    }
    else if ((msCount % GYRO_CHECK_MS) == 0){
    
      
      //test();
    }
  }
  return 0;
}