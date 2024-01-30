#include "sam.h"
#include "gyro.h"
#include <stddef.h>
#include <stdbool.h>

#if !defined(I2C)
#define I2C
#define I2C_BAUD (64U)
#define WRITE false
#define READ true

void clkI2C();
void initI2C();
void sercom2Init();

//I2C transaction functions
void i2cSndAddr(uint8_t addr, bool dir);
void initTx(uint8_t addr, size_t bytes,volatile unsigned char *data, bool dir);
void initRead(uint8_t addr, size_t bytes,volatile unsigned char *data);
#endif // I2C
