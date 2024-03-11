#include "sam.h"
#include <stdbool.h>
//HEAVY influence from the following repo
//https://github.com/majbthrd/SAMC21demoCAN

#if !defined(CAN)
#define CAN

#define QUANTA_BEFORE 5UL
#define QUANTA_AFTER 7UL
#define QUANTA_SYNC 2UL

#define MSG_LIST_SIZE 25UL 
#define MEM_SIZE (MSG_LIST_SIZE*sizeof(uint32_t))
#define RX_HEADER_SZ (2u)

//The message struct from the sample code seems like a great idea
//So I am making my own heavily inspired by it
typedef struct can_msg{

    uint32_t id;
    uint32_t time;
    uint8_t *data;
    uint8_t len;
    uint8_t dataLen;

} CAN_MSG;

//Here we initalize arrays to use for our buffers and we will give these addresses to the 
//CAN controller registers to use DMA
static uint32_t msgRam[MSG_LIST_SIZE] = {1};
static uint32_t *msgAddr = msgRam;
static uint32_t txRam[MSG_LIST_SIZE];
static uint32_t *txAddr = txRam;
static uint32_t txEvent[MSG_LIST_SIZE];
static uint32_t *eventAddr = txEvent; 
static uint32_t rxRam[MSG_LIST_SIZE]; //Should i times 2 so I can initate both rx fifos?
static uint32_t *rxAddr = rxRam;
static uint32_t rxBuff[MSG_LIST_SIZE]; //Should i times 2 so I can initate both rx fifos?
static uint32_t *rxBuffAddr = rxBuff;
static uint32_t currTxIndex = 0;
//init can
void clkCAN();
void initCAN(uint32_t *ram, uint32_t *tx, uint32_t *rx, uint32_t *event, uint32_t *rxbuff);
void CAN0Init();

//rx and filter
void setFifoFilter(int fifo, uint8_t filter, uint32_t id, uint32_t mask);
void getCanRxBuffData(uint8_t index, CAN_MSG *msg, uint32_t *ramAddr);
bool hasRxBuffData(uint8_t index);

//tx 3
void sendCanTXbuffer(uint8_t index);
void enqueueCanTxMsg(uint32_t id, uint8_t length, const uint8_t *data);
void dequeueCanTxMsg();


#endif // CAN
