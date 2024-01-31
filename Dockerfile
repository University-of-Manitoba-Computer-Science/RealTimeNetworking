FROM debian:bullseye-slim

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    libncurses-dev \
    python3 \
    python3-pip \
    python3-setuptools \
    python3-wheel \
    && rm -rf /var/lib/apt/lists/*

# Install the latest version of ARM GCC Toolchain 
ADD https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-aarch64-arm-none-eabi.tar.xz /tmp/arm-none-eabi.tar.xz
# ADD https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz /tmp/arm-none-eabi.tar.xz

RUN tar -xf /tmp/arm-none-eabi.tar.xz -C /opt \
    && rm /tmp/arm-none-eabi.tar.xz \
    && mv /opt/arm-gnu-toolchain-* /opt/arm-none-eabi

ENV PATH="/opt/arm-none-eabi/bin:${PATH}"

# Install the latest version of OpenOCD
ADD https://github.com/xpack-dev-tools/openocd-xpack/releases/download/v0.12.0-2/xpack-openocd-0.12.0-2-linux-arm64.tar.gz /tmp/openocd.tar.gz
# ADD https://github.com/xpack-dev-tools/openocd-xpack/releases/download/v0.12.0-2/xpack-openocd-0.12.0-2-linux-x64.tar.gz /tmp/openocd.tar.gz

RUN tar -xf /tmp/openocd.tar.gz -C /opt \
    && rm /tmp/openocd.tar.gz \
    && mv /opt/xpack-openocd-* /opt/openocd

ENV PATH="/opt/openocd/bin:${PATH}"

# Set the working directory
WORKDIR /usr/src/app

# Copy the current directory contents into the container
COPY . .

RUN ["make", "blinky-install"]