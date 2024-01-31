# Compiler
CC := arm-none-eabi-gcc

# Device specific configuration
DEVICE := same51j20a
CPU := Cortex-M4
BOARD := microchip_same51_curiosity_nano
PACK := E51-Pack
DEVICE_UPPER := $(shell echo $(DEVICE) | tr '[:lower:]' '[:upper:]')

# Directories
BUILD_DIR := bin
SRC_DIR := src
INC_DIR := includes

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.c)
LIBS := $(wildcard $(INC_DIR)/*.c)

# Include directories
INCLUDE_DIRS := -Ivendor/$(PACK)/include -Ivendor/Core/Include -I$(INC_DIR)

# Compiler flags
CFLAGS := -x c -mthumb -mcpu=$(CPU) -D__$(DEVICE_UPPER)__ -O1 -ffunction-sections -Wall -c -std=gnu99
LDFLAGS := -Wl,--start-group -lm -Wl,--end-group -Wl,--gc-sections -mthumb -mcpu=$(CPU) -Tvendor/$(PACK)/gcc/gcc/$(DEVICE)_flash.ld

# Object files
SYS_OBJS := vendor/$(PACK)/gcc/system_$(DEVICE).o vendor/$(PACK)/gcc/gcc/startup_$(DEVICE).o
LIB_OBJS := $(patsubst $(INC_DIR)/%.c,$(BUILD_DIR)/lib/%.o,$(LIBS))
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/build/%.o,$(SRCS))

# Targets
.PHONY: all clean

all: $(BUILD_DIR)/blinky.elf

debug: CFLAGS += -g
debug: clean all
release: CFLAGS += -DNDEBUG
release: clean all

$(BUILD_DIR)/build/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $< -o $@

$(BUILD_DIR)/lib/%.o: $(INC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $< -o $@

%.o: %.c 
	$(CC) $(CFLAGS) -Ivendor/$(PACK)/include -Ivendor/Core/Include $*.c -o $*.o

$(BUILD_DIR)/%.elf: $(SYS_OBJS) $(LIB_OBJS) $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(SYS_OBJS) $(LIB_OBJS) $(OBJS)
	arm-none-eabi-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature $@ $(BUILD_DIR)/$*.hex
	arm-none-eabi-objdump -h -S $@ > $(BUILD_DIR)/$*.lss
	arm-none-eabi-size $@

clean:
	rm -rf $(BUILD_DIR)

# Example install target - modify as needed
%-install: $(BUILD_DIR)/%.elf
	openocd -f board/$(BOARD).cfg -c "program $(BUILD_DIR)/$*.elf verify reset exit"
	