#include "sam.h"

//HEAVY influence from the following repo
//https://github.com/majbthrd/SAMC21demoCAN

#if !defined(CAN)
#define CAN

#define QUANTA_BEFORE 5UL
#define QUANTA_AFTER 7UL
#define QUANTA_SYNC 2UL

#define MSG_LIST_SIZE 25

void clkCAN();
void initCAN(uint32_t *ram, uint32_t *tx, uint32_t *rx, uint32_t *event, uint32_t *rxbuff);
void CAN0Init();

void txData();


#endif // CAN
