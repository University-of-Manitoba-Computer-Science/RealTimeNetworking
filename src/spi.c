#include "spi.h"
#include "sam.h"
#include "pins.h"

void init_spi()
{
    // configure the pin directions
    PORT_REGS->GROUP[0].PORT_DIRSET = SPI0_CS_Msk;

    // configure SPI pins
    PORT_REGS->GROUP[0].PORT_PINCFG[SPI0_MOSI] = PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[0].PORT_PINCFG[SPI0_MISO] = PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[0].PORT_PINCFG[SPI0_SCK] = PORT_PINCFG_PMUXEN_Msk;
    // PORT_REGS->GROUP[0].PORT_PINCFG[SS] = PORT_PINCFG_PMUXEN_Msk;

    PORT_REGS->GROUP[0].PORT_PMUX[8] = PORT_PMUX_PMUXE_C | PORT_PMUX_PMUXO_C;
    PORT_REGS->GROUP[0].PORT_PMUX[9] = PORT_PMUX_PMUXO_C;

    MCLK_REGS->MCLK_APBAMASK |= MCLK_APBAMASK_SERCOM1_Msk;
    GCLK_REGS->GCLK_PCHCTRL[8] |= GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[8] & GCLK_PCHCTRL_CHEN_Msk) == 0)
        ;

    // configure the SPI peripheral to use 8-bit data and enable the receiver
    SERCOM1_REGS->SPIM.SERCOM_CTRLB = SERCOM_SPIM_CTRLB_CHSIZE_8_BIT | SERCOM_SPIM_CTRLB_RXEN_Msk;
    while ((SERCOM1_REGS->SPIM.SERCOM_SYNCBUSY) != 0U)
        ;

    // fref = 120MHz, fbaud = 30MHz, BAUD = fref / (2 * fbaud) - 1
    // set BAUD to 1 to get 30MHz baud rate, or
    SERCOM1_REGS->SPIM.SERCOM_BAUD = (uint8_t)SERCOM_SPIM_BAUD_BAUD(1UL);

    // configure the SPI peripheral to be a master, use default pads, use spi mode 0, data order msb, and enable the peripheral
    SERCOM1_REGS->SPIM.SERCOM_CTRLA = SERCOM_SPIM_CTRLA_MODE_SPI_MASTER | SERCOM_SPIM_CTRLA_DOPO_PAD0 | SERCOM_SPIM_CTRLA_DIPO_PAD3 | SERCOM_SPIM_CTRLA_CPOL_IDLE_LOW | SERCOM_SPIM_CTRLA_CPHA_LEADING_EDGE | SERCOM_SPIM_CTRLA_DORD_MSB | SERCOM_SPIM_CTRLA_ENABLE_Msk;
    while ((SERCOM1_REGS->SPIM.SERCOM_SYNCBUSY) != 0U)
        ;
}

uint8_t spi_write(uint8_t *data, uint8_t len)
{
    // iterate over the data bytes and write them to the SPI data register
    for (uint8_t i = 0; i < len; i++)
    {
        // wait for the data register to be empty
        while ((SERCOM1_REGS->SPIM.SERCOM_INTFLAG & SERCOM_SPIM_INTFLAG_DRE_Msk) == 0U)
            ;

        // write the data to the SPI data register
        SERCOM1_REGS->SPIM.SERCOM_DATA = (uint8_t)data[i];

        // wait for the transfer to complete
        while ((SERCOM1_REGS->SPIM.SERCOM_INTFLAG & SERCOM_SPIM_INTFLAG_TXC_Msk) == 0U)
            ;
    }
    return 0;
}

uint8_t spi_read(uint8_t *data, uint8_t len)
{
    // iterate through each byte
    for (uint8_t i = 0; i < len; i++)
    {
        // wait for the data register to be empty
        while ((SERCOM1_REGS->SPIM.SERCOM_INTFLAG & SERCOM_SPIM_INTFLAG_DRE_Msk) == 0U)
            ;

        // write a dummy byte to the SPI data register
        SERCOM1_REGS->SPIM.SERCOM_DATA = 0;

        // wait for the transmisson to complete
        while ((SERCOM1_REGS->SPIM.SERCOM_INTFLAG & SERCOM_SPIM_INTFLAG_TXC_Msk) == 0U)
            ;

        // wait for the data to be received
        while ((SERCOM1_REGS->SPIM.SERCOM_INTFLAG & SERCOM_SPIM_INTFLAG_RXC_Msk) == 0U)
            ;

        // read the data from the SPI data register
        data[i] = (uint8_t)SERCOM1_REGS->SPIM.SERCOM_DATA;
    }
    return 0;
}

// select the device by setting the CS pin low
void spi_select_device(uint32_t pin)
{
    PORT_REGS->GROUP[0].PORT_OUTCLR = pin;
}

// deselect the device by setting the CS pin high
void spi_deselect_device(uint32_t pin)
{
    PORT_REGS->GROUP[0].PORT_OUTSET = pin;
}