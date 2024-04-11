# COMP 4060 - T29 - Embedded Realtime Networking
## Description
The project implemented a real-time network with 3 curiosity nano boards connected over a CAN network, UART network and utilized a Wifi8 click to incorporate wifi capabilities. Commands were issued over wifi and sent to the middle board to be translated into a UART message which was sent to a sensor board with a fan, and LSM9DS1 gyro from ada fruit. This sensor board would either adjust the fan speed or send a buffered accelerometer or gyro meter back to the board with the wifi via the translation board. 
## Contents
The project contains the following  
- CAN drivers
- UART drivers
- I2C drivers modified for the LSM9DS1 Gyro
- WIFI drivers for Wifi8 click
- PWMing code for a fan along with rpm feedback
## Contributors
- [Landon Colburn](https://github.com/landoncolburn)
- [Jacob Janzen](https://github.com/JacobJanzen)
- [Andrew Marinic](https://github.com/AMarinic92)
