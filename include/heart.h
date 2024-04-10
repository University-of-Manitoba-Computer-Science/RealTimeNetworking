#ifndef HEART_H_
#define HEART_H_

#include <sam.h>
#include <stdint.h>

uint32_t get_ticks();
void delay_ms(uint32_t ms);
void heartInit();

#endif // HEART_H_
