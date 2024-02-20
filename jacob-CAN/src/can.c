#include "can.h"

#include <sam.h>
#include <string.h>

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
#define RX_FIFO0_ELEMENT_SIZE      18
#define RX_FIFO1_ELEMENT_SIZE      18
#define RX_BUFFER_ELEMENT_SIZE     18
#define TX_EVENT_FIFO_ELEMENT_SIZE 2
#define TX_BUFFER_ELEMENT_SIZE     18

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

    // setup clock
    GCLK_REGS->GCLK_PCHCTRL[27] = GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[27] & GCLK_PCHCTRL_CHEN_Msk) !=
           GCLK_PCHCTRL_CHEN_Msk)
        ;
    MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_CAN0_Msk;

    // Configuration
    CAN0_REGS->CAN_CCCR |= CAN_CCCR_INIT_Msk; // start initialization
    CAN0_REGS->CAN_CCCR |= CAN_CCCR_CCE_Msk;  // enable configuration

    // TODO remove once CAN driver is working over loopback
    CAN0_REGS->CAN_CCCR |= CAN_CCCR_TEST_Msk; // Enable test mode
    CAN0_REGS->CAN_TEST  = CAN_TEST_LBCK_Msk; // Enable loopback mode

    // Reject non-matching frames
    // TODO I don't know if we need this, but testing some filtering seems like
    // a good idea
    CAN0_REGS->CAN_GFC |= CAN_GFC_ANFS(2) | CAN_GFC_ANFE(2);

    // TODO do we need to set bit rate? Using defaults for now

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
    CAN0_REGS->CAN_TXBC = CAN_TXBC_TBSA(addr) | CAN_TXBC_NDTB(TX_BUFFER_SIZE) |
                          CAN_TXBC_TFQS(TX_BUFFER_SIZE);

    // unset CCCR.INIT once synchronized
    while ((CAN0_REGS->CAN_CCCR & CAN_CCCR_INIT_Msk) != CAN_CCCR_INIT_Msk)
        ;
    CAN0_REGS->CAN_CCCR &= ~CAN_CCCR_INIT_Msk;
}

// TODO do we even need more than one TX slot?
void send_message(uint8_t *data, int len, int buf_i, int id)
{
    // find start offset of buffer index
    // TODO 4 doesn't seem correct, but it is the only value that works
    // seems like only 2 bytes are allowed in each queue element, weird...
    int offset = TX_BUFFER_OFFSET + (buf_i * 4);

    // set the first word of the header
    msg_ram[offset++] = STD_BUF_ID(id);

    // set the second word of the header
    msg_ram[offset++] = MM_BUF(0) | DLC_BUF((uint32_t) len);

    // set data field of message
    memcpy(&msg_ram[offset], data, len);

    // queue the message to send
    CAN0_REGS->CAN_TXBAR |= 1 << buf_i;
}
