#ifndef CAN_H_
#define CAN_H_

#include <stdint.h>

void canInit(void);
void put_message(uint8_t *data, int len);

#endif // CAN_H_
