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
