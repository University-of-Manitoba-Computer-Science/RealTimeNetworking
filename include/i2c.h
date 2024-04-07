#include "sam.h"
#include <stddef.h>
#include <stdbool.h>
#include "same51j20a.h"


#define I2C_BAUD (1U)
#define WRITE false
#define READ true

#define GYRO_I2C_AG (0x6B)
#define GYRO_I2C_M (0x1D)
#define CTRL_REG6_XL (0x20)
#define CTRL_REG6_XL_10HZ_Msk (0x20) //set the output data rate and power mode to 10hz and let bandwidth be automatic XL mode only
#define CTRL_REG6_XL_50HZ_Msk (0x40) //set the output data rate and power mode to 50hz and let bandwidth be automatic XL mode only
#define OUT_X_XL_START (0x28) //start at lsb 
#define OUT_Y_XL_START (0X2A)
#define OUT_Z_XL_START (0X2C)
#define CTRL_REG8 (0x22)
#define CTRL_REG8_IF_ADD_INC_Msk (0X04) //Set the IF_ADD_INC bit on the CTRL_REG8
#define AUTO_INC_Msk (0x80) //Or this to sub addresses to do auto increment on the address.
#define READ_BUF_SIZE 1000
#define WRITE_BUF_SIZE 32



void clkI2C();
void initI2C();
void sercom2Init();

//I2C transaction functions
void i2cSndAddr(uint8_t addr, bool dir);
void initTx(uint8_t addr, size_t bytes,volatile unsigned char *data, bool dir);
void initRead(uint8_t addr, size_t bytes,volatile unsigned char *data);

void accelOnlyMode();
void sampleXL();

