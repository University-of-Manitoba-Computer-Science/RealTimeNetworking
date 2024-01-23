# default target when "make" is run w/o arguments
all: blinky.elf

# device specific naming and (sub)paths
DEVICE=same51j20a
CPU=Cortex-M4
BOARD=microchip_same51_curiosity_nano
PACK=E51-pack

# standard compiling setup
CC=arm-none-eabi-gcc
DEVICE_UPPER=$(shell echo $(DEVICE) | tr  '[:lower:]' '[:upper:]')

INCLUDE_PATHS=-I../$(PACK)/include -I../Core/include -Iinclude
ASFLAGS=-mthumb -mcpu=$(CPU) -D__$(DEVICE_UPPER)__ -O1 -ffunction-sections -Wall
CFLAGS=-x c -mthumb -mcpu=$(CPU) -D__$(DEVICE_UPPER)__ -O1 -ffunction-sections -Wall -c -std=gnu99
LDFLAGS=-Wl,--start-group -lm  -Wl,--end-group -Wl,--gc-sections -mthumb -mcpu=$(CPU) -T../$(PACK)/gcc/gcc/$(DEVICE)_flash.ld 

SYS_OBJS=../$(PACK)/gcc/system_$(DEVICE).o ../$(PACK)/gcc/gcc/startup_$(DEVICE).o dcc_stdio.o

debug: CFLAGS += -g
debug: clean all
release: CFLAGS += -DNDEBUG
release: clean all

.PRECIOUS: %.o

# build all sources needed by the project
%.o: %.c 
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) $*.c -o $*.o

# build the executable file
%.elf: %.o $(SYS_OBJS)
	$(CC) -o $*.elf $(LDFLAGS) $(SYS_OBJS) $*.o
	arm-none-eabi-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature  $*.elf $*.hex
	arm-none-eabi-objdump -h -S $*.elf > $*.lss
	arm-none-eabi-size $*.elf

clean:
	rm -f *.o *.elf *.hex *.lss

# put the executable on the device
%-install: %.elf
	openocd -f board/$(BOARD).cfg -c "program $*.elf verify reset exit"
