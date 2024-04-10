#include "sam.h"
#include <stdint.h>

#define NUM_SAMPLES 16
#define EXTINT14_MASK (0x4000) // ie 1 << 14

void updateOutput(uint8_t dutyCycle);

void fanInit();

void rpmInit();

volatile uint16_t getRpm();

// clock is setup to ensure that it never overflows within our RPM range; an overflow indicates no pulse, or that the fan has stopped
void TCC0_OTHER_Handler();


// we do need averaging as there is some jitter in the feedback from the fan

// ISR for match capture events
void TCC0_MC0_Handler();