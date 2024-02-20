#ifndef CAN_H_
#define CAN_H_

#include <stdint.h>

void canInit(void);
void send_message(uint8_t *data, int len, int buf_i);

#endif // CAN_H_
