file build/CAN-RS485.elf
target extended-remote | openocd -f board/microchip_same51_curiosity_nano.cfg -c 'gdb_port pipe'
monitor target_request debugmsgs enable
monitor trace point 1
monitor reset halt
continue
