#include "can.h"

#include <sam.h>
#include <string.h>

#include "dcc_stdio.h"

#define STD_BUF_ID_Pos    18
#define STD_BUF_ID_Msk    (0x7ffu << STD_BUF_ID_Pos)
#define STD_BUF_ID(value) (STD_BUF_ID_Msk & (_UINT32_(value) << STD_BUF_ID_Pos))

#define MM_BUF_Pos    24
#define MM_BUF_Msk    (0xffu << MM_BUF_Pos)
#define MM_BUF(value) ((MM_BUF_Msk & ((value) << MM_BUF_Pos)))

#define DLC_BUF_Pos    16
#define DLC_BUF_Msk    (0xfu << DLC_BUF_Pos)
#define DLC_BUF(value) ((DLC_BUF_Msk & ((value) << DLC_BUF_Pos)))

#define STD_FILT_ELEMENT_SIZE      1
#define EXT_FILT_ELEMENT_SIZE      2
#define RX_FIFO0_ELEMENT_SIZE      4
#define RX_FIFO1_ELEMENT_SIZE      4
#define RX_BUFFER_ELEMENT_SIZE     4
#define TX_EVENT_FIFO_ELEMENT_SIZE 2
#define TX_BUFFER_ELEMENT_SIZE     4

#define STD_FILT_SIZE      128
#define EXT_FILT_SIZE      64
#define RX_FIFO0_SIZE      64
#define RX_FIFO1_SIZE      64
#define RX_BUFFER_SIZE     64
#define TX_EVENT_FIFO_SIZE 32
#define TX_BUFFER_SIZE     32

#define STD_FILT_OFFSET 0
#define EXT_FILT_OFFSET                                                        \
    (STD_FILT_OFFSET) + (STD_FILT_SIZE) * (STD_FILT_ELEMENT_SIZE)
#define RX_FIFO0_OFFSET                                                        \
    (EXT_FILT_OFFSET) + (EXT_FILT_SIZE) * (EXT_FILT_ELEMENT_SIZE)
#define RX_FIFO1_OFFSET                                                        \
    (RX_FIFO0_OFFSET) + (RX_FIFO0_SIZE) * (RX_FIFO0_ELEMENT_SIZE)
#define RX_BUFFER_OFFSET                                                       \
    (RX_FIFO1_OFFSET) + (RX_FIFO1_SIZE) * (RX_FIFO1_ELEMENT_SIZE)
#define TX_EVENT_FIFO_OFFSET                                                   \
    (RX_BUFFER_OFFSET) + (RX_BUFFER_SIZE) * (RX_BUFFER_ELEMENT_SIZE)
#define TX_BUFFER_OFFSET                                                       \
    (TX_EVENT_FIFO_OFFSET) + (TX_EVENT_FIFO_SIZE) * (TX_EVENT_FIFO_ELEMENT_SIZE)
#define MSG_RAM_SIZE                                                           \
    (TX_BUFFER_OFFSET) + (TX_BUFFER_SIZE) * (TX_BUFFER_ELEMENT_SIZE)

static uint32_t msg_ram[MSG_RAM_SIZE];

void canInit()
{
    // setup TX port
    PORT_REGS->GROUP[0].PORT_DIRSET      = PORT_PA22;
    PORT_REGS->GROUP[0].PORT_PINCFG[22] |= PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[0].PORT_PMUX[11]   |= PORT_PMUX_PMUXE_I;

    // setup RX port
    PORT_REGS->GROUP[0].PORT_DIRCLR      = PORT_PA23;
    PORT_REGS->GROUP[0].PORT_PINCFG[23] |= PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[0].PORT_PMUX[11]   |= PORT_PMUX_PMUXO_I;

    // setup mode port
    PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA20;
    PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA20;

    // set generator 3 to use DFLL48M as a source; with a divider of 3 that
    // gives us 16 MHz
    // TODO experimentally found that a division factor of 3 gives 1 MHz CAN
    // output. I do not know why this works
    GCLK_REGS->GCLK_GENCTRL[3] =
        GCLK_GENCTRL_DIV(3) | GCLK_GENCTRL_SRC_DFLL | GCLK_GENCTRL_GENEN_Msk;
    while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL_GCLK3) ==
           GCLK_SYNCBUSY_GENCTRL_GCLK3); /* Wait for synchronization */

    // setup clock
    GCLK_REGS->GCLK_PCHCTRL[27] =
        GCLK_PCHCTRL_GEN_GCLK3 | GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[27] & GCLK_PCHCTRL_CHEN_Msk) !=
           GCLK_PCHCTRL_CHEN_Msk);
    MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_CAN0_Msk;

    // Configuration
    CAN0_REGS->CAN_CCCR |= CAN_CCCR_INIT_Msk; // start initialization
    CAN0_REGS->CAN_CCCR |= CAN_CCCR_CCE_Msk;  // enable configuration

    // TODO remove once CAN driver is working over loopback
    // TODO this prevents the endless stream of TX
    CAN0_REGS->CAN_CCCR |= CAN_CCCR_TEST_Msk; // Enable test mode
    CAN0_REGS->CAN_TEST  = CAN_TEST_LBCK_Msk; // Enable loopback mode

    // Accept non-matching standard frames in FIFO0
    // Reject non-matching extended frames
    CAN0_REGS->CAN_GFC |= CAN_GFC_ANFS(0) | CAN_GFC_ANFE(2);

    // Setting up message RAM
    uint32_t *addr = msg_ram;

    // Standard filter RAM conf
    CAN0_REGS->CAN_SIDFC = CAN_SIDFC_FLSSA(addr) | CAN_SIDFC_LSS(STD_FILT_SIZE);
    addr += STD_FILT_SIZE * STD_FILT_ELEMENT_SIZE;

    // Extended filter RAM conf
    CAN0_REGS->CAN_XIDFC = CAN_XIDFC_FLESA(addr) | CAN_XIDFC_LSE(EXT_FILT_SIZE);
    addr += EXT_FILT_SIZE * EXT_FILT_ELEMENT_SIZE;

    // RX FIFO 0 RAM conf
    CAN0_REGS->CAN_RXF0C = CAN_RXF0C_F0OM_Msk | CAN_RXF0C_F0SA(addr) |
                           CAN_RXF0C_F0S(RX_FIFO0_SIZE);
    addr += RX_FIFO0_SIZE * RX_FIFO0_ELEMENT_SIZE;

    // RX FIFO 1 RAM conf
    CAN0_REGS->CAN_RXF1C = CAN_RXF1C_F1OM_Msk | CAN_RXF1C_F1SA(addr) |
                           CAN_RXF1C_F1S(RX_FIFO1_SIZE);
    addr += RX_FIFO1_SIZE * RX_FIFO1_ELEMENT_SIZE;

    // RX Buffer RAM conf
    CAN0_REGS->CAN_RXBC  = CAN_RXBC_RBSA(addr);
    addr                += RX_BUFFER_SIZE * RX_BUFFER_ELEMENT_SIZE;
    // TODO why isn't there an option to set the buffer size?

    // TX Event FIFO RAM conf
    CAN0_REGS->CAN_TXEFC =
        CAN_TXEFC_EFSA(addr) | CAN_TXEFC_EFS(TX_EVENT_FIFO_SIZE);
    addr += TX_EVENT_FIFO_SIZE * TX_EVENT_FIFO_ELEMENT_SIZE;

    // TX Buffer RAM conf
    CAN0_REGS->CAN_TXBC =
        CAN_TXBC_TBSA(addr) | CAN_TXBC_NDTB(0) | CAN_TXBC_TFQS(TX_BUFFER_SIZE);

    for (int i = 0; i < TX_BUFFER_SIZE; ++i) CAN0_REGS->CAN_TXBTIE |= (1 << i);

    // unset CCCR.INIT once synchronized
    while ((CAN0_REGS->CAN_CCCR & CAN_CCCR_INIT_Msk) != CAN_CCCR_INIT_Msk);
    CAN0_REGS->CAN_CCCR &= ~CAN_CCCR_INIT_Msk;

    PORT_REGS->GROUP[0].PORT_OUTCLR = PORT_PA20;
}

void put_message(uint8_t *data, int len)
{
    if ((CAN0_REGS->CAN_TXFQS & CAN_TXFQS_TFQF_Msk) == CAN_TXFQS_TFQF_Msk)
        return; // don't send message if the queue is full

    uint32_t status = CAN0_REGS->CAN_TXFQS & CAN_TXFQS_TFQPI_Msk;
    uint8_t  index  = status >> CAN_TXFQS_TFQPI_Pos;
    dbg_write_u8(&index, 1);
    dbg_write_char('\n');

    int offset        = TX_BUFFER_OFFSET + (index * TX_BUFFER_ELEMENT_SIZE);
    msg_ram[offset++] = STD_BUF_ID(0x123);
    msg_ram[offset++] = MM_BUF(0) | DLC_BUF((uint32_t) 1);
    memcpy(&msg_ram[offset], &index, 1);

    CAN0_REGS->CAN_TXBAR |= 1 << index;
}

int dequeue_message(uint8_t *data, int max_size)
{
    int status = CAN0_REGS->CAN_RXF0S;

    int fill_level = (status & CAN_RXF0S_F0FL_Msk) >> CAN_RXF0S_F0FL_Pos;
    int get_index  = (status & CAN_RXF0S_F0GI_Msk) >> CAN_RXF0S_F0GI_Pos;

    // exit on empty FIFO
    if (fill_level == 0) {
        return -1;
    }

    // acknowledge
    CAN0_REGS->CAN_RXF0A = CAN_RXF0A_F0AI(get_index);

    int offset = RX_FIFO0_OFFSET + (get_index * RX_FIFO0_ELEMENT_SIZE);

    int line0 = msg_ram[offset++];
    int line1 = msg_ram[offset++];

    dbg_write_u32(&line0, 1);
    dbg_write_u32(&line1, 1);

    // TODO get message length
    // TODO copy payload into data

    return 0;
}
