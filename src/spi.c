#include "spi.h"
#include "sam.h"
#include "pins.h"

// see https://github.com/MikroElektronika/mikrosdk_v2/blob/a0ba439bfa61be8f176762efc9e715a61a84d2d7/drv/lib/include/drv_spi_master.h#L423

#define DUMMY 0x00

void init_spi()
{
    // configure the pin directions
    // PORT_REGS->GROUP[0].PORT_DIRSET |= SPI0_CS_Msk;
    // PORT_REGS->GROUP[0].PORT_OUTSET |= SPI0_CS_Msk;

    // configure SPI pins
    PORT_REGS->GROUP[0].PORT_PINCFG[SPI0_MOSI] = PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[0].PORT_PINCFG[SPI0_MISO] = PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[0].PORT_PINCFG[SPI0_SCK] = PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[0].PORT_PINCFG[SPI0_CS] = PORT_PINCFG_PMUXEN_Msk;

    PORT_REGS->GROUP[0].PORT_PMUX[8] = PORT_PMUX_PMUXE_C | PORT_PMUX_PMUXO_C;
    PORT_REGS->GROUP[0].PORT_PMUX[9] = PORT_PMUX_PMUXO_C | PORT_PMUX_PMUXE_C;

    // configure generic clock generator 3 to use the DFLL
    GCLK_REGS->GCLK_GENCTRL[3] = GCLK_GENCTRL_DIV(48) | GCLK_GENCTRL_SRC_DFLL | GCLK_GENCTRL_GENEN_Msk;
    while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL_GCLK3) ==
           GCLK_SYNCBUSY_GENCTRL_GCLK3)
        ;

    GCLK_REGS->GCLK_PCHCTRL[SERCOM1_GCLK_ID_CORE] = GCLK_PCHCTRL_GEN_GCLK3 | GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[SERCOM1_GCLK_ID_CORE] & GCLK_PCHCTRL_CHEN_Msk) == 0)
        ;
    MCLK_REGS->MCLK_APBAMASK |= MCLK_APBAMASK_SERCOM1_Msk;

    // configure the SPI peripheral to use 8-bit data and enable the receiver
    SERCOM1_REGS->SPIM.SERCOM_CTRLB = SERCOM_SPIM_CTRLB_CHSIZE_8_BIT | SERCOM_SPIM_CTRLB_RXEN_Msk | SERCOM_SPIM_CTRLB_MSSEN_Msk;
    while ((SERCOM1_REGS->SPIM.SERCOM_SYNCBUSY) != 0U)
        ;

    SERCOM1_REGS->SPIM.SERCOM_BAUD = (uint8_t)SERCOM_SPIM_BAUD_BAUD(255);

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

uint8_t spi_io(void *tx_data, size_t tx_size, void *rx_data, size_t rx_size)
{
    size_t tx_count = 0;
    size_t rx_count = 0;
    size_t diff_size = 0;
    size_t received_data;

    // Verify the request
    if ((tx_size <= 0 || !tx_data) && (rx_size == 0 || !rx_data))
        return -1;

    if (!tx_data)
    {
        tx_size = 0;
    }

    if (!rx_data)
    {
        rx_size = 0;
    }

    // Flush out unread data in data register from previous transfer
    while ((SERCOM1_REGS->SPIM.SERCOM_INTFLAG & SERCOM_SPIM_INTFLAG_RXC_Msk) ==
           SERCOM_SPIM_INTFLAG_RXC_Msk)
    {
        received_data = SERCOM1_REGS->SPIM.SERCOM_DATA;
    }

    // clear interrupt flags
    SERCOM1_REGS->SPIM.SERCOM_STATUS |=
        (uint16_t)SERCOM_SPIM_STATUS_BUFOVF_Msk;
    SERCOM1_REGS->SPIM.SERCOM_INTFLAG |=
        (uint8_t)SERCOM_SPIM_INTFLAG_ERROR_Msk;

    if (rx_size > tx_size)
    {
        diff_size = rx_size - tx_size;
    }

    // Make sure DRE is empty
    while ((SERCOM1_REGS->SPIM.SERCOM_INTFLAG & SERCOM_SPIM_INTFLAG_DRE_Msk) !=
           SERCOM_SPIM_INTFLAG_DRE_Msk)
        ;

    // while there's stuff to transmit or receive, handle it
    while (tx_count != tx_size || diff_size != 0)
    {
        if (tx_count != tx_size)
        {
            SERCOM1_REGS->SPIM.SERCOM_DATA = ((uint8_t *)tx_data)[tx_count];
            tx_count++;
        }
        else if (diff_size > 0)
        {
            SERCOM1_REGS->SPIM.SERCOM_DATA = DUMMY;

            diff_size--;
        }

        if (rx_size == 0)
        {
            // For transmit only request, wait for DRE to become empty
            while ((SERCOM1_REGS->SPIM.SERCOM_INTFLAG &
                    SERCOM_SPIM_INTFLAG_DRE_Msk) != SERCOM_SPIM_INTFLAG_DRE_Msk)
                ;
        }
        else
        {
            // If data is read, wait for the rx data register to fill
            while ((SERCOM1_REGS->SPIM.SERCOM_INTFLAG &
                    SERCOM_SPIM_INTFLAG_RXC_Msk) != SERCOM_SPIM_INTFLAG_RXC_Msk)
                ;

            received_data = SERCOM1_REGS->SPIM.SERCOM_DATA;

            if (rx_count < rx_size)
            {
                ((uint8_t *)rx_data)[rx_count++] = (uint8_t)received_data;
            }
        }
    }

    // Make sure no data is still in the shift register
    while ((SERCOM1_REGS->SPIM.SERCOM_INTFLAG & SERCOM_SPIM_INTFLAG_TXC_Msk) !=
           SERCOM_SPIM_INTFLAG_TXC_Msk)
        ;

    return 0;
}

// select the device by setting the CS pin low
void spi_select_device(uint32_t pin)
{
    // PORT_REGS->GROUP[0].PORT_OUTCLR |= pin;
}

// deselect the device by setting the CS pin high
void spi_deselect_device(uint32_t pin)
{
    // PORT_REGS->GROUP[0].PORT_OUTSET |= pin;
}