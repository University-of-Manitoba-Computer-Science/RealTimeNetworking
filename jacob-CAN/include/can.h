#ifndef CAN_H_
#define CAN_H_

#include <stdint.h>

void canInit(void);
void queue_message(uint8_t *data, int len);
int  dequeue_message(uint8_t *data, int max_len);

#endif // CAN_H_
