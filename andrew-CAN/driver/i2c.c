#include "i2c.h"
void clkI2C(){




}//clkI2C

void initI2C(){



}//initI2C

void sercom2Init(){
    //I2C SDA on PORT_PA12
    PORT_REGS->GROUP[0].PORT_PINCFG[12] |= PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[0].PORT_PMUX[6] |= PORT_PMUX_PMUXE_C;

    //I2C SCL on PORT_PA13
    PORT_REGS->GROUP[0].PORT_PINCFG[13] |= PORT_PINCFG_PMUXEN_Msk;
    PORT_REGS->GROUP[0].PORT_PMUX[6] |= PORT_PMUX_PMUXO_C;

}//sercom2Init
