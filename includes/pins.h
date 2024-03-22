/*
| Pin Name  | Pin Number | Pin Function | Pin Description     |
| --------- | ---------- | ------------ | ------------------- |
| SPI0_EN   | PA06       | EN           | Chip Enable         |
| SPI0_RST  | PA22       | RST          | Reset               |
| SPI0_CS   | PA23       | CS           | Chip Select         |
| SPI0_SCK  | PA17       | SCK          | Serial Clock        |
| SPI0_MISO | PA19       | MISO/SDO     | Master In Slave Out |
| SPI0_MOSI | PA16       | MOSI/SDI     | Master Out Slave In |
| SPI0_INT  | PA09       | INT          | Interrupt           |
*/

#include "sam.h"

#define SPI0_EN PIN_PA02
#define SPI0_RST PIN_PA07
#define SPI0_CS PIN_PA18
#define SPI0_SCK PIN_PA17
#define SPI0_MISO PIN_PA19
#define SPI0_MOSI PIN_PA16
#define SPI0_INT PIN_PA04

#define SPI0_EN_Msk PORT_PA02
#define SPI0_RST_Msk PORT_PA07
#define SPI0_CS_Msk PORT_PA18
#define SPI0_SCK_Msk PORT_PA17
#define SPI0_MISO_Msk PORT_PA19
#define SPI0_MOSI_Msk PORT_PA16
#define SPI0_INT_Msk PORT_PA04
