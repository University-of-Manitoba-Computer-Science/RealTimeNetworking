/*
| Pin Name  | Pin Number | Pin Function | Pin Description     |
| --------- | ---------- | ------------ | ------------------- |
| WIFI0_EN   | PA06       | EN           | Chip Enable         |
| WIFI0_RST  | PA22       | RST          | Reset               |
| WIFI0_CS   | PA23       | CS           | Chip Select         |
| WIFI0_SCK  | PA17       | SCK          | Serial Clock        |
| WIFI0_MISO | PA19       | MISO/SDO     | Master In Slave Out |
| WIFI0_MOSI | PA16       | MOSI/SDI     | Master Out Slave In |
| WIFI0_INT  | PA09       | INT          | Interrupt           |
*/

#include "sam.h"

#define WIFI0_EN PIN_PA02
#define WIFI0_RST PIN_PA07
#define WIFI0_CS PIN_PA18
#define WIFI0_SCK PIN_PA17
#define WIFI0_MISO PIN_PA19
#define WIFI0_MOSI PIN_PA16
#define WIFI0_INT PIN_PA04

#define WIFI0_EN_Msk PORT_PA02
#define WIFI0_RST_Msk PORT_PA07
#define WIFI0_CS_Msk PORT_PA18
#define WIFI0_SCK_Msk PORT_PA17
#define WIFI0_MISO_Msk PORT_PA19
#define WIFI0_MOSI_Msk PORT_PA16
#define WIFI0_INT_Msk PORT_PA04
