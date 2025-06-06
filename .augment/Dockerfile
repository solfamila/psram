# Dockerfile for Augment Remote Agent - Embedded C Development
# Provides consistent build environment for MIMXRT700 project

FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    wget \
    curl \
    unzip \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install ARM GCC toolchain
RUN wget -q https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 \
    && tar -xjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /opt/ \
    && rm gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2

# Add ARM GCC to PATH
ENV PATH="/opt/gcc-arm-none-eabi-10.3-2021.10/bin:${PATH}"
ENV ARMGCC_DIR="/opt/gcc-arm-none-eabi-10.3-2021.10"

# Install Unity testing framework for embedded C
RUN git clone https://github.com/ThrowTheSwitch/Unity.git /opt/unity

# Set working directory
WORKDIR /workspace

# Copy project files
COPY . .

# Set environment variables for the project
ENV SDK_ROOT="/workspace/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__"

# Default command
CMD ["/bin/bash"]
