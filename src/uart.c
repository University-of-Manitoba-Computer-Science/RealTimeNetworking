#include "uart.h"

// reference
// https://microchip-mplab-harmony.github.io/reference_apps/apps/sam_e51_cnano/same51n_getting_started/readme.html

#define BUFFER_SIZE 16

// define circular queue with a fixed maximum size
// overwrite old messages if the queue is full
typedef struct RX_BUFFER {
    int     num_messages;
    int     start_index;
    uint8_t buffer[BUFFER_SIZE];
} rx_buffer_t;

rx_buffer_t sercom0_buf = {
    .num_messages = 0,
    .start_index  = 0,
};

rx_buffer_t sercom4_buf = {
    .num_messages = 0,
    .start_index  = 0,
};

static void dequeue_uart(sercom_registers_t *sercom, rx_buffer_t *buf)
{
    // temporarily disable interrupt
    sercom->USART_INT.SERCOM_INTENCLR = SERCOM_USART_INT_INTENSET_RXC_Msk;

    if (sercom->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_RXC_Msk) {
        buf->buffer[buf->start_index + buf->num_messages] =
            sercom->USART_INT.SERCOM_DATA;
        buf->num_messages = buf->num_messages >= BUFFER_SIZE
                                ? BUFFER_SIZE
                                : buf->num_messages + 1;
    }

    // reenable interrupt
    sercom->USART_INT.SERCOM_INTENSET = SERCOM_USART_INT_INTENSET_RXC_Msk;
}

void SERCOM0_2_Handler()
{
    dequeue_uart(SERCOM0_REGS, &sercom0_buf);
}

void SERCOM4_2_Handler()
{
    dequeue_uart(SERCOM4_REGS, &sercom4_buf);
}

void clkUART(sercom_registers_t *sercom)
{
    // DFLL0 not DPLL
    // I want the UART on an 8mhz clock for maybe uneducated reasons (I want it
    // to get 1mbps transmit speed)
    GCLK_REGS->GCLK_GENCTRL[4] =
        GCLK_GENCTRL_DIV(3) | GCLK_GENCTRL_SRC_DFLL | GCLK_GENCTRL_GENEN_Msk;

    if (sercom == SERCOM0_REGS) {
        GCLK_REGS->GCLK_PCHCTRL[SERCOM0_GCLK_ID_CORE] =
            GCLK_PCHCTRL_GEN_GCLK4 | GCLK_PCHCTRL_CHEN_Msk;

        while ((GCLK_REGS->GCLK_PCHCTRL[SERCOM0_GCLK_ID_CORE] &
                GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk) {
            // wait for sync
        } // while
        MCLK_REGS->MCLK_APBAMASK |= MCLK_APBAMASK_SERCOM0_Msk;
    } else if (sercom == SERCOM4_REGS) {
        GCLK_REGS->GCLK_PCHCTRL[SERCOM4_GCLK_ID_CORE] =
            GCLK_PCHCTRL_GEN_GCLK4 | GCLK_PCHCTRL_CHEN_Msk;
        while ((GCLK_REGS->GCLK_PCHCTRL[SERCOM4_GCLK_ID_CORE] &
                GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk) {
            // wait for sync
        } // while
        MCLK_REGS->MCLK_APBDMASK |= MCLK_APBDMASK_SERCOM4_Msk;
    }

} // clkUART

void initUART(sercom_registers_t *sercom)
{
    // The following comes from section 34.6.2.1
    // disable the USART sercom
    if (sercom == SERCOM0_REGS) {
        SERCOM0_REGS->USART_INT.SERCOM_CTRLA = SERCOM_USART_INT_CTRLA_ENABLE(0);
    }
    if (sercom == SERCOM4_REGS) {
        SERCOM4_REGS->USART_INT.SERCOM_CTRLA = SERCOM_USART_INT_CTRLA_ENABLE(0);
    }

    // todo: I have changed how TXP0 works along with RXP0 to select PAD1 for RX
    // as per the spec, but I need to make sure the TXP0 mode change from 0x0 to
    // 0x2 works The following sets up CTRLA as follows selects async mode, with
    // internal clock, RX (PAD1) and TX (PAD0) on SERCOM0 with internal clock,
    // sets to MSB mode Not sure if we want TXP0 as mode 0x0 (sercom pad 0 for
    // rx but pad 1 gets used for exck) or mode 0x2 where we dont use pad 1 for
    // anything else.

    // The following sets up CTRLB as follows
    sercom->USART_INT.SERCOM_CTRLB = SERCOM_USART_INT_CTRLB_CHSIZE_8_BIT |
                                     SERCOM_USART_INT_CTRLB_SBMODE_1_BIT |
                                     SERCOM_USART_INT_CTRLB_TXEN_Msk |
                                     SERCOM_USART_INT_CTRLB_RXEN_Msk;
    while ((sercom->USART_INT.SERCOM_SYNCBUSY &
            SERCOM_USART_INT_SYNCBUSY_CTRLB_Msk) != 0) {
        // Wait for CTRLB enable
    } // while

    // Since we are using internal clock we can set desired baud rate with our
    // 8Mhz clock (gclk4)
    sercom->USART_INT.SERCOM_BAUD = SERCOM_USART_INT_BAUD_BAUD(UART_BUAD);

    // enable transmit and receive

    // Enable sercom
    if (sercom == SERCOM0_REGS) {
        sercom->USART_INT.SERCOM_CTRLA =
            SERCOM_USART_INT_CTRLA_MODE_USART_INT_CLK |
            SERCOM_USART_INT_CTRLA_RXPO(SERCOM0_RX_MODE) |
            SERCOM_USART_INT_CTRLA_TXPO(SERCOM0_TX_MODE) |
            SERCOM_USART_INT_CTRLA_DORD_Msk | SERCOM_USART_INT_CTRLA_ENABLE_Msk;
    } else if (sercom == SERCOM4_REGS) {
        sercom->USART_INT.SERCOM_CTRLA =
            SERCOM_USART_INT_CTRLA_MODE_USART_INT_CLK |
            SERCOM_USART_INT_CTRLA_RXPO(SERCOM4_RX_MODE) |
            SERCOM_USART_INT_CTRLA_TXPO(SERCOM4_TX_MODE) |
            SERCOM_USART_INT_CTRLA_DORD_Msk | SERCOM_USART_INT_CTRLA_ENABLE_Msk;
    }
    while ((sercom->USART_INT.SERCOM_SYNCBUSY &
            SERCOM_USART_INT_SYNCBUSY_ENABLE_Msk) != 0) {
        // Wait for enable
    } // while

} // initUART

void portUART(sercom_registers_t *sercom)
{
    if (sercom == SERCOM0_REGS) {
        // USART sercom0 on PORT_PA8
        PORT_REGS->GROUP[0].PORT_PINCFG[8] |= PORT_PINCFG_PMUXEN_Msk;
        PORT_REGS->GROUP[0].PORT_PMUX[4]   |= PORT_PMUX_PMUXE_C;

        // USART sercom0 on PORT_PA9
        PORT_REGS->GROUP[0].PORT_PINCFG[9] |= PORT_PINCFG_PMUXEN_Msk;
        PORT_REGS->GROUP[0].PORT_PMUX[4]   |= PORT_PMUX_PMUXO_C;

        // PB05 for one DE for pad 2 SERCOM0
        PORT_REGS->GROUP[1].PORT_DIRSET = PORT_PB05;
        PORT_REGS->GROUP[1].PORT_OUTSET = PORT_PB05;

        // PB14 for RE for pad 2
        PORT_REGS->GROUP[1].PORT_DIRSET = PORT_PB14;
        PORT_REGS->GROUP[1].PORT_OUTSET = PORT_PB14;

    } else if (sercom == SERCOM4_REGS) {
        // USART SERCOM4 on PORT_PA12
        PORT_REGS->GROUP[1].PORT_PINCFG[12] |= PORT_PINCFG_PMUXEN_Msk;
        PORT_REGS->GROUP[1].PORT_PMUX[6]    |= PORT_PMUX_PMUXE_C;

        // USART SERCOM4 on PORT_PA13
        PORT_REGS->GROUP[1].PORT_PINCFG[13] |= PORT_PINCFG_PMUXEN_Msk;
        PORT_REGS->GROUP[1].PORT_PMUX[6]    |= PORT_PMUX_PMUXO_C;

        // PA04 for one DE FOR PAD 1 SERCOM4
        PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA04;
        PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA04;

        // P18 for RE FOR PAD 1
        PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA18;
        PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA18;
    }

} // portUART

void rxMode(sercom_registers_t *sercom)
{
    if (sercom == SERCOM0_REGS) {
        // PB14 and PB05for RE and DE for pad 2

        PORT_REGS->GROUP[1].PORT_OUTCLR = PORT_PB14 | PORT_PB05;

        // enable interrupts
        NVIC_EnableIRQ(SERCOM0_2_IRQn);
        SERCOM0_REGS->USART_INT.SERCOM_INTENSET =
            SERCOM_USART_INT_INTENSET_RXC_Msk;
    } // if
    else if (sercom == SERCOM4_REGS) {
        // PA04 for one DE FOR PAD 1 SERCOM4

        PORT_REGS->GROUP[0].PORT_OUTCLR = PORT_PA04 | PORT_PA18;

        // enable interrupts
        NVIC_EnableIRQ(SERCOM4_2_IRQn);
        SERCOM4_REGS->USART_INT.SERCOM_INTENSET =
            SERCOM_USART_INT_INTENSET_RXC_Msk;
    } // else if

} // rxMode

void txMode(sercom_registers_t *sercom)
{
    if (sercom == SERCOM0_REGS) {
        // PB14 and PB05 for RE and DE for pad 2

        PORT_REGS->GROUP[1].PORT_OUTSET = PORT_PB14 | PORT_PB05;

    } // if
    else if (sercom == SERCOM4_REGS) {
        // PA04 for one DE FOR PAD 1 SERCOM4

        PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA04 | PORT_PA18;

    } // else if

} // txMode

void txUART(sercom_registers_t *sercom, uint8_t data)
{
    txMode(sercom);

    while ((sercom->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk
           ) == 0U) {
        // WAIT for clear int flag

    } // while

    sercom->USART_INT.SERCOM_DATA = (uint16_t) data;

    uint32_t timestamp = 1;
    while ((timestamp % TX_DELAY_MS) != 0) {
        timestamp++;

    } // while

} // txUART

void txUARTArr(sercom_registers_t *sercom, uint8_t *data, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        txUART(sercom, data[i]);

    } // for

    uint32_t timestamp = 1;
    while ((timestamp % TX_DELAY_MS) != 0) {
        timestamp++;
    } // while

} // txUARTArr

uint8_t rxUART(sercom_registers_t *sercom, uint8_t *buffer, int buffer_size)
{
    // determine buffer to read from
    rx_buffer_t *buf;
    if (sercom == SERCOM0_REGS) {
        buf = &sercom0_buf;
    } else if (sercom == SERCOM4_REGS) {
        buf = &sercom4_buf;
    } else {
        return 0;
    }

    // temporarily disable interrupt
    sercom->USART_INT.SERCOM_INTENCLR = SERCOM_USART_INT_INTENSET_RXC_Msk;

    // compute size of buffer to read
    int size = 0;
    size = buf->num_messages > buffer_size ? buffer_size : buf->num_messages;

    // read until return buffer is full
    for (int i = 0; i < size; ++i) {
        buffer[i]        = buf->buffer[buf->start_index];
        buf->start_index = (buf->start_index + 1) % BUFFER_SIZE;
        --(buf->num_messages);
    }

    // reenable interrupt
    sercom->USART_INT.SERCOM_INTENSET = SERCOM_USART_INT_INTENSET_RXC_Msk;

    return size;
}
