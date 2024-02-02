FROM debian:bullseye-slim

# Set the architecture for the ARM GCC Toolchain and OpenOCD
ARG GCC_ARCH=aarch64
ARG OPENOCD_ARCH=arm64
# ARG GCC_ARCH=x86_64
# ARG OPENOCD_ARCH=x64

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    unzip \
    && rm -rf /var/lib/apt/lists/*

# Install the latest version of ARM GCC Toolchain 
ADD https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-${GCC_ARCH}-arm-none-eabi.tar.xz /tmp/arm-none-eabi.tar.xz

# Extract the toolchain and move it to /opt
RUN tar -xf /tmp/arm-none-eabi.tar.xz -C /opt \
    && rm /tmp/arm-none-eabi.tar.xz \
    && mv /opt/arm-gnu-toolchain-* /opt/arm-none-eabi

# Add the toolchain to the PATH
ENV PATH="/opt/arm-none-eabi/bin:${PATH}"

# Install the latest version of OpenOCD
ADD https://github.com/xpack-dev-tools/openocd-xpack/releases/download/v0.12.0-2/xpack-openocd-0.12.0-2-linux-${OPENOCD_ARCH}.tar.gz /tmp/openocd.tar.gz

# Extract OpenOCD and move it to /opt
RUN tar -xf /tmp/openocd.tar.gz -C /opt \
    && rm /tmp/openocd.tar.gz \
    && mv /opt/xpack-openocd-* /opt/openocd

# Add OpenOCD to the PATH
ENV PATH="/opt/openocd/bin:${PATH}"

# Install CMSIS Pack
ADD https://github.com/ARM-software/CMSIS_5/releases/download/5.9.0/ARM.CMSIS.5.9.0.pack /tmp/cmsis.pack

# Extract CMSIS Pack
RUN mkdir -p /tmp/cmsis/ \
    && unzip /tmp/cmsis.pack -d /tmp/cmsis \
    && mkdir -p /opt/core/ \
    && mv /tmp/cmsis/CMSIS/Core/* /opt/core/ \
    && rm -rf /tmp/cmsis.pack /tmp/cmsis

# Install SAME51 Pack
ADD https://packs.download.microchip.com/Microchip.SAME51_DFP.3.7.242.pack /tmp/same51.pack

# Extract SAME51 Pack
RUN mkdir -p /opt/same51 \
    && unzip /tmp/same51.pack -d /opt/same51 \
    && rm -rf /tmp/same51.pack

