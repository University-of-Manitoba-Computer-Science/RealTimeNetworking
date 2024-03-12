#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <stdbool.h>

#define delay_1ms() delay_ms(1)
void delay_ms(uint32_t ms);
bool testLedTimer();
void heartInit();

#endif