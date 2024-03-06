#include "sam.h"
#include <stdbool.h>
//HEAVY influence from the following repo
//https://github.com/majbthrd/SAMC21demoCAN

#if !defined(CAN)
#define CAN

#define QUANTA_BEFORE 5UL
#define QUANTA_AFTER 7UL
#define QUANTA_SYNC 2UL

#define MSG_LIST_SIZE 25
#define MEM_SIZE MSG_LIST_SIZE*sizeof(uint32_t)

//The message struct from the sample code seems like a great idea
//So I am making my own heavily inspired by it
typedef struct can_msg{

    uint32_t id;
    uint32_t time;
    uint8_t *data;
    uint8_t len;
    uint8_t data_len;

} CAN_MSG;

//init can
void clkCAN();
void initCAN(uint32_t *ram, uint32_t *tx, uint32_t *rx, uint32_t *event, uint32_t *rxbuff);
void CAN0Init();

//rx and filter
void setFifoFilter(int fifo, uint8_t filter, uint32_t id, uint32_t mask);
bool getCanRxBuffData(uint8_t index);

//tx 3
void sendCanTXbuffer(uint8_t index);
void enqueueCanTxMsg(uint32_t id, uint8_t length, const uint8_t *data);



#endif // CAN
