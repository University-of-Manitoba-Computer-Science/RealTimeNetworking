# Tue 19 Dec 2023 23:53:21 CST

Built project repository and installed toolchain. Still waiting to get hardware from my Dad's house

# Mon 8 Jan 2024 10:08:45 CST

Got project structure setup, modified and built blinky, and uploaded to board.

# Wed 10 Jan 2024 13:20:23 CST

Tested our debugging with VSCode debugger and gdb. Tested assertations and printing.

# Mon 15 Jan 2024 18:04:12 CST

Research and planning on CAN network, setup CAN SFU on SAMEJ20A dev kit.

# Fri 26 Jan 2024 13:05:18 CST

Moved code to new repo, restructured project and built branch.

# Fri 26 Jan 2024 22:01:09 CST

Reorged project structure (again...) to use directories and cleaned up code. Still trying to fix verify issue that occurs on the first build

# Wed 31 Jan 2024 13:19:26 CST

Added docker build method to solve dependency issues with python version and ncurses. Also bumped gcc version since ubuntu repo uses old version from 2018 which resulted in a .elf file roughly ~10x in size

# Wed 31 Jan 2024 13:41:16 CST

Added comments for x86_64 arch, added OpenOCD, waiting to test usb passthrough

# Fri Feb 2 14:45:58 CST 2024

Happy with my docker build system, reached out to Professor Zapp to get urls for CMSIS pack/core so they can be pulled automatically in dockerfile which is now working, also allowed dockerfile to recieve args to determine system arch instead of relying on commented urls

# Fri Feb 2 14:53:17 CST 2024

Slightly changed Makefile because dynamic install is not important for this situation

# Sat Feb 3 12:23:45 CST 2024

Added CMSIS pack and core to dockerfile, also added a script to pull the pack and core from the url and extract it to the correct location

# Mon Feb 5 10:11:14 CST 2024

Worked more on intializing CAN, cleaned up some other code to make expanding slighly easier

# Fri Feb 9 18:34:59 CST 2024

Finally finished build system (makefile and docker) and configured the rest of my vscode to work with the new build system

# Wed Feb 28 17:00:00 CST 2024

Had issues with CAN bus click and spent a week debugging it. Got stomach flu and took week off. Contacted Professor Zapp and we decided to shift to focus on wi-fi click and get it working because power issues were interfering with CAN board progress (was not able to flash the board anymore)

# Mon Mar 4 13:54:21 CST 2024

Spent a couple days working on the wifi8 click, tried finding documentation but most is either for the ATWINC3400-MR210CA module itself or the Mikroe SDK. Ended up learning from the SDK code

# Fri Mar 10 10:45:39 CST 2024

Got some of the wifi click working, am having some other SPI issues now though. Was informed by group that we are allowed to use the SDK itself since it's almost 5k lines of code and has VERY little documentation, so I began modifying the SDK to get it to work with our project (remove the rest of the SDK and kept the wifi click code). I am working on getting proper and reliable SPI communication with the wifi click, my success criteria is to be able to pull the firmware versions from the chip and then I can start using the lib to create the network and broadcast the data. I had to change all of the SPI code, and remove some references to other parts of the SDK.

# Sun Mar 17 21:37:09 CDT 2024

Made large progress. SPI communication from SAME51 board to wifi8 click looks good, I captured communication with the logic analyzer. I am still not getting a reply from the click board, and I suspect it is power related. I found similar issues on forums online with the ATWIN chip. My next step is to power the board externally and test for any problems.

# Week of Mar 18 2024

I spent the week trying to figure out what is going on with my SPI communication. First off I thought it was power issues since it should be a very simple implementation, but after talking to Professor Zapp he seems to thing the issue is somewhere else. I then assumed that maybe my transactions were not being recognized since the CS line is being flipped on and off too quickly due to the fast system clock, but that also seems unlikely since the transactions shouldn't be responsible for anything meaningful in the communication process anyways. I tried flipping the MISO and MOSI lines and I recieved some response from the board, but it doesn't match the data I expected and it also doesn't match the schematic so it's unlikely to be true progress. I flipped them back and removed delays I added to test marking the end of transactions. I also tried using Jacob's SPI code that has been confirmed working on other projects with both send and recieve (mine only was tested with send, recieve was purely based off theory). None of these changes ended up getting a response from the board so I am meeting with Zapp on Friday in his office to scope the board and figure out what is going on with the click. I will update the log again after our meeting

# March 23 2024

We got two way communication working! We moved everything to the baseboard and used the reccomended pins and switched the CS pin to be hardware controlled and everything worked like a charm! I was able to initialize the device and pull down firmware versions. I had to make some other changes to facilitate the rest of the communication (including the interrupt pin, which is a fake interrupt currently and seemingly is in their API as well). But I was able to successfully authenticate to my WiFi and get assigned an IP address. I attempted to send over the socket but the board seems to be having power issues again. I am waiting to move to a Mikroe Base Board to hopefully get proper voltage regulation on Monday.
