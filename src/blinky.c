#include "sam.h"
#include "dcc_stdio.h"
#include "heartbeat.h"
#include "spi.h"
#include "wifi8.h"
#include <string.h>
#include <assert.h>

#define EXTINT15_MASK 0x8000

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

// button press handler
void EIC_EXTINT_15_Handler()
{
  EIC_REGS->EIC_INTFLAG |= EXTINT15_MASK;
}

uint8_t count = 0;

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

  // initialize the SPI
  init_spi();

  // wait for wifi
  delay_ms(1000);

  // initialize the wifi
  wifi8_t ctx;
  err_t result = wifi8_init(&ctx);

  // wait for the on-board debugger USB UART to be ready
  delay_ms(1000);

  // print status
  printf("wifi8_init: %d\n", result);

  wifi8_m2m_rev_t fw_version;
  if (WIFI8_OK == wifi8_get_full_firmware_version(&ctx, &fw_version))
  {
    printf("Firmware HIF (%u) : %u.%u \n",
           ((uint16_t)(((fw_version.u16_firmware_hif_info) >> (14)) & (0x3))),
           ((uint16_t)(((fw_version.u16_firmware_hif_info) >> (8)) & (0x3f))),
           ((uint16_t)(((fw_version.u16_firmware_hif_info) >> (0)) & (0xff))));
    printf("Firmware ver   : %u.%u.%u \n",
           (uint16_t)fw_version.u8_firmware_major,
           (uint16_t)fw_version.u8_firmware_minor,
           (uint16_t)fw_version.u8_firmware_patch);
    printf("Firmware Build %s Time %s\n", fw_version.build_date, fw_version.build_time);
  }
  else
  {
    printf("error reading full firmware version\n");
    for (;;)
      ;
  }

  // sleep until we have an interrupt
  while (1)
  {
    __WFI();

    if (testLedTimer())
    {
      PORT_REGS->GROUP[0].PORT_OUTTGL = PORT_PA14;
    }
  }
  return 0;
}
