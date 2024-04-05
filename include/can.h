#ifndef CAN_H_
#define CAN_H_

#include <stdint.h>

void canInit(uint16_t *ids_to_keep, int num_ids_to_keep);
void queue_message(uint16_t id, uint8_t *data, int len);
int  dequeue_message(uint8_t *data, int max_len);

#endif // CAN_H_
