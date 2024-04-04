#include "sam.h"

#if !defined(BUTTON)
#define BUTTON

#define EXTINT15_MASK 0x8000

void clkButton();
void initButton();
void port15Init();



#endif // BUTTON
