# Name of entry file
MAIN := blinky

# Compiler
CC := arm-none-eabi-gcc

# Device specific configuration
DEVICE := same51j20a
CPU := Cortex-M4
BOARD := microchip_same51_curiosity_nano
DEVICE_UPPER := $(shell echo $(DEVICE) | tr '[:lower:]' '[:upper:]')

# Vendor paths (override as needed)
CORE_PATH ?= ../Core
PACK_PATH ?= ../E51-pack

# Directories
BUILD_DIR := bin
SRC_DIR := src
INC_DIR := includes

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.c)
LIBS := $(wildcard $(INC_DIR)/*.c)

# Include directories
INCLUDE_DIRS := -I$(PACK_PATH)/include -I$(CORE_PATH)/Include -I$(INC_DIR)

# Compiler flags
CFLAGS := -x c -mthumb -mcpu=$(CPU) -D__$(DEVICE_UPPER)__ -O1 -ffunction-sections -Wall -c -std=gnu99
LDFLAGS := -Wl,--start-group -lm -Wl,--end-group -Wl,--gc-sections -mthumb -mcpu=$(CPU) -T$(PACK_PATH)/gcc/gcc/$(DEVICE)_flash.ld

# Object files
SYS_OBJS := $(PACK_PATH)/gcc/system_$(DEVICE).o $(PACK_PATH)/gcc/gcc/startup_$(DEVICE).o
LIB_OBJS := $(patsubst $(INC_DIR)/%.c,$(BUILD_DIR)/lib/%.o,$(LIBS))
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/build/%.o,$(SRCS))

# Targets
.PHONY: all clean

# Default target
all: $(BUILD_DIR)/$(MAIN).elf

# Debug and release targets
debug: CFLAGS += -g
debug: clean all
release: CFLAGS += -DNDEBUG
release: clean all

# Rule for compiling project C files
$(BUILD_DIR)/build/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $< -o $@

# Rule for compiling library C files
$(BUILD_DIR)/lib/%.o: $(INC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $< -o $@

# Rule for compiling misc C files
%.o: %.c 
	$(CC) $(CFLAGS) -I$(PACK_PATH)/include -I$(CORE_PATH)/Include $*.c -o $*.o

# Rule for linking the project
$(BUILD_DIR)/%.elf: $(SYS_OBJS) $(LIB_OBJS) $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(SYS_OBJS) $(LIB_OBJS) $(OBJS)
	arm-none-eabi-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature $@ $(BUILD_DIR)/$*.hex
	arm-none-eabi-objdump -h -S $@ > $(BUILD_DIR)/$*.lss
	arm-none-eabi-size $@

# Clean up
clean:
	rm -rf $(BUILD_DIR)

# Example install target - modify as needed
install: $(BUILD_DIR)/$(MAIN).elf
	openocd -f board/$(BOARD).cfg -c "program $(BUILD_DIR)/$(MAIN).elf verify reset exit"

# Build the docker container based build environment and tag it as supercoolrealtimenetworkingbuilder:latest
docker-build:
	docker build -t supercoolrealtimenetworkingbuilder:latest .

# Run the build target in the docker container
docker-install: docker-build
	docker run --rm -it \
		--privileged \
		-v ./:/app \
		-w /app \
		supercoolrealtimenetworkingbuilder:latest \
		make install CORE_PATH=/opt/core PACK_PATH=/opt/same51
	