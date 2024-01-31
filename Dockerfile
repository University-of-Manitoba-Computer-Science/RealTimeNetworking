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

RUN tar -xf /tmp/arm-none-eabi.tar.xz -C /opt \
    && rm /tmp/arm-none-eabi.tar.xz \
    && mv /opt/arm-gnu-toolchain-* /opt/arm-none-eabi

ENV PATH="/opt/arm-none-eabi/bin:${PATH}"

# Set the working directory
WORKDIR /usr/src/app

# Copy the current directory contents into the container
COPY . .

RUN ["make", "release"]