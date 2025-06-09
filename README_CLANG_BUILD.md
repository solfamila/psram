# MIMXRT700 XSPI PSRAM Project - LLVM/Clang Build Guide

This guide provides complete instructions for building the MIMXRT700 XSPI PSRAM project using LLVM/Clang instead of the default ARM GCC toolchain.

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Step 1: Install Prerequisites](#step-1-install-prerequisites)
4. [Step 2: Download and Build LLVM/Clang](#step-2-download-and-build-llvmclang)
5. [Step 3: Install ARM GCC Toolchain](#step-3-install-arm-gcc-toolchain)
6. [Step 4: Set Up Project Configuration](#step-4-set-up-project-configuration)
7. [Step 5: Build the Project](#step-5-build-the-project)
8. [Results Comparison](#results-comparison)
9. [Troubleshooting](#troubleshooting)
10. [Technical Details](#technical-details)

## Overview

This project demonstrates how to compile an embedded ARM Cortex-M33 project (MIMXRT798S) using LLVM/Clang instead of the traditional ARM GCC toolchain. The setup uses:

- **LLVM/Clang 19.1.6** - Modern C/C++ compiler with excellent optimization
- **LLD Linker** - LLVM's fast linker
- **ARM GCC Libraries** - Embedded C library and runtime support
- **MCUXpresso SDK** - NXP's hardware abstraction layer

### Benefits of Using Clang

- **Better optimization**: Often produces smaller and faster code
- **Superior diagnostics**: More helpful error messages and warnings
- **Modern tooling**: Integration with static analysis tools
- **Cross-platform**: Consistent behavior across different host platforms

## Prerequisites

### System Requirements

- **Disk Space**: ~10GB free space for LLVM build
- **Memory**: 8GB+ RAM recommended for LLVM compilation
- **CPU**: Multi-core processor (build time: 30-60 minutes)
- **OS**: Linux (Ubuntu/Debian), macOS, or Windows with WSL

### Required Tools

- Git
- CMake (3.10 or later)
- Ninja build system
- Python 3
- C/C++ compiler (for building LLVM)

## Step 1: Install Prerequisites

### Ubuntu/Debian

```bash
sudo apt update
sudo apt install git cmake ninja-build python3 build-essential wget curl
```

### macOS

```bash
# Install Homebrew if you don't have it
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install tools
brew install git cmake ninja python3 wget
```

### Windows

1. Install Visual Studio 2019/2022 with C++ support
2. Install Git for Windows
3. Install CMake from cmake.org
4. Install Python 3 from python.org
5. Consider using WSL2 for easier setup

## Step 2: Download and Build LLVM/Clang

### 2.1 Download LLVM Source Code

```bash
# Create a temporary directory for the build
cd /tmp

# Download LLVM 19.1.6 source code (~500MB)
wget -O llvm-project-19.1.6.tar.gz \
  https://github.com/llvm/llvm-project/archive/refs/tags/llvmorg-19.1.6.tar.gz

# Extract the source code
tar -xzf llvm-project-19.1.6.tar.gz
```

### 2.2 Configure LLVM Build

```bash
# Create build directory
mkdir -p llvm-build
cd llvm-build

# Configure LLVM with ARM support
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/opt/llvm-19.1.6 \
  -DLLVM_ENABLE_PROJECTS="clang;lld" \
  -DLLVM_TARGETS_TO_BUILD="ARM;AArch64;X86" \
  -DLLVM_DEFAULT_TARGET_TRIPLE="arm-none-eabi" \
  -DLLVM_ENABLE_ASSERTIONS=OFF \
  -DLLVM_OPTIMIZED_TABLEGEN=ON \
  -DLLVM_PARALLEL_LINK_JOBS=2 \
  -DLLVM_PARALLEL_COMPILE_JOBS=4 \
  ../llvm-project-llvmorg-19.1.6/llvm
```

**Configuration Notes:**
- Adjust `LLVM_PARALLEL_COMPILE_JOBS` based on your CPU cores
- Keep `LLVM_PARALLEL_LINK_JOBS=2` if you have <16GB RAM
- Change install prefix if you prefer a different location

### 2.3 Build LLVM (30-60 minutes)

```bash
# Build with appropriate parallelism for your system
ninja -j4  # Adjust -j based on your CPU cores

# For systems with limited RAM, use fewer jobs:
# ninja -j2
```

### 2.4 Install LLVM

```bash
# Install to /opt/llvm-19.1.6
sudo ninja install
```

### 2.5 Verify Installation

```bash
# Check Clang version
/opt/llvm-19.1.6/bin/clang --version

# Verify ARM target support
/opt/llvm-19.1.6/bin/clang --print-targets | grep arm
```

Expected output should show ARM targets including `arm` and `armeb`.

## Step 3: Install ARM GCC Toolchain

The ARM GCC toolchain provides the embedded C library and runtime support needed for bare-metal ARM development.

### 3.1 Download ARM GCC

```bash
cd /tmp

# Download ARM GCC 10.3-2021.10 (adjust URL for your platform)
# For Linux x86_64:
wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2

# For macOS:
# wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-mac.tar.bz2
```

### 3.2 Install ARM GCC

```bash
# Extract to /opt
sudo tar -xjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /opt/

# Verify installation
/opt/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-gcc --version
```

## Step 4: Set Up Project Configuration

### 4.1 Clone the Project

```bash
cd ~/workspace  # or your preferred directory
git clone https://github.com/solfamila/psram.git
cd psram
```

### 4.2 Create Configuration Files

Create the following files in the `mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc/` directory:

1. **clang_toolchain.cmake** - CMake toolchain configuration
2. **CMakeLists_clang.txt** - Clang-specific CMake build file
3. **flags_clang.cmake** - Compiler and linker flags
4. **debug_console_stub.c** - Stub functions for disabled components

Copy the complete content from the provided configuration files (see separate files in this package).

### 4.3 Update Paths (if necessary)

If you installed LLVM or ARM GCC to different locations, update the paths in:

- `clang_toolchain.cmake`: Update `ARM_GCC_SYSROOT` and compiler paths
- `flags_clang.cmake`: Update `ARM_GCC_SYSROOT` and library paths

## Step 5: Build the Project

### 5.1 Manual Build Process

```bash
cd mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc

# Create build directory
mkdir -p build_clang_debug
cd build_clang_debug

# Copy configuration files
cp ../CMakeLists_clang.txt CMakeLists.txt
cp ../clang_toolchain.cmake .
cp ../flags_clang.cmake .
cp ../config.cmake .
cp ../*.ld .
cp ../../../debug_console_stub.c .

# Configure with CMake
cmake -DCMAKE_TOOLCHAIN_FILE=clang_toolchain.cmake \
      -DCMAKE_BUILD_TYPE=debug \
      .

# Build the project
make -j4
```

### 5.2 Using the Build Script

Alternatively, use the provided build script:

```bash
# Make the script executable
chmod +x build_clang_complete.sh

# Build debug version
./build_clang_complete.sh debug

# Build release version
./build_clang_complete.sh release

# Clean build
./build_clang_complete.sh debug clean
```

### 5.3 Build Output

Successful build will produce:
- `debug/xspi_psram_polling_transfer_cm33_core0.elf` - ELF executable
- `debug/xspi_psram_polling_transfer.bin` - Binary for flashing
- `output.map` - Memory map file

## Results Comparison

### Build Size Comparison

| Metric | ARM GCC | LLVM Clang | Difference |
|--------|---------|------------|------------|
| **Binary Size** | 53 KB | 44 KB | **-17% (Clang smaller)** |
| **ELF Size** | 528 KB | 466 KB | -12% (Clang smaller) |
| **Text Size** | 35,696 B | 39,816 B | +12% (Clang slightly larger) |
| **Flash Usage** | 2.52% | 1.91% | -24% (Clang more efficient) |

### Performance Benefits

- **Smaller binary size**: Clang produces a 17% smaller binary
- **Better optimization**: More efficient flash usage
- **Modern toolchain**: Access to advanced LLVM tools and analysis

## Troubleshooting

### Common Issues and Solutions

#### 1. "Clang not found" Error

**Problem**: CMake cannot find the Clang compiler.

**Solution**:
```bash
# Verify Clang installation
ls -la /opt/llvm-19.1.6/bin/clang

# Update paths in clang_toolchain.cmake if installed elsewhere
```

#### 2. "ARM GCC libraries not found" Error

**Problem**: Clang cannot find ARM embedded libraries.

**Solution**:
```bash
# Verify ARM GCC installation
ls -la /opt/gcc-arm-none-eabi-10.3-2021.10/

# Check library paths
find /opt/gcc-arm-none-eabi-10.3-2021.10 -name "libc.a"

# Update ARM_GCC_SYSROOT in configuration files
```

#### 3. Large Binary Size (>100MB)

**Problem**: Binary file is extremely large due to memory layout issues.

**Solution**: Ensure the objcopy command in CMakeLists_clang.txt includes only flash sections:
```cmake
--only-section=.flash_config --only-section=.interrupts --only-section=.text --only-section=.ARM --only-section=.ctors --only-section=.dtors --only-section=.data
```

#### 4. Linker Errors about Missing Symbols

**Problem**: LLD linker cannot find entry point or system symbols.

**Solution**: Check that flags_clang.cmake includes the symbol definitions:
```cmake
-Wl,--entry=Reset_Handler
-Wl,--defsym=__main=main
-Wl,--defsym=_start=main
```

#### 5. Memory Issues During LLVM Build

**Problem**: System runs out of memory during LLVM compilation.

**Solutions**:
- Reduce parallel jobs: `ninja -j2` instead of `ninja -j4`
- Add swap space: `sudo fallocate -l 4G /swapfile && sudo mkswap /swapfile && sudo swapon /swapfile`
- Use a machine with more RAM

#### 6. Permission Denied Errors

**Problem**: Cannot write to `/opt/` directory.

**Solution**:
```bash
# Use sudo for installation
sudo ninja install

# Or install to user directory
cmake -DCMAKE_INSTALL_PREFIX=$HOME/llvm-19.1.6 ...
```

### Platform-Specific Notes

#### macOS

- Use `/usr/local/opt/llvm-19.1.6` instead of `/opt/llvm-19.1.6`
- ARM GCC path will be different
- May need to install Xcode command line tools

#### Windows

- Use Windows paths in configuration files
- Consider using WSL2 for easier setup
- Visual Studio may be required for some dependencies

## Technical Details

### Why This Setup Works

1. **Clang Cross-Compilation**: Uses `--target arm-none-eabi` to generate ARM code
2. **ARM Libraries**: Leverages ARM GCC's embedded C library and runtime
3. **LLD Linker**: LLVM's linker with ARM support and custom linker scripts
4. **MCUXpresso SDK**: Full compatibility with NXP's hardware abstraction layer

### Key Configuration Elements

- **Target Triple**: `arm-none-eabi` for bare-metal ARM
- **CPU Specification**: `cortex-m33` with hard float ABI
- **Sysroot**: Points to ARM GCC's embedded libraries
- **Runtime Library**: Uses `libgcc` instead of Clang's compiler-rt

### Files Created/Modified

1. `clang_toolchain.cmake` - CMake cross-compilation setup
2. `CMakeLists_clang.txt` - Clang-specific build configuration
3. `flags_clang.cmake` - Compiler and linker flags
4. `debug_console_stub.c` - Stub implementations for disabled features
5. `build_clang_complete.sh` - Automated build script
6. `README_CLANG_BUILD.md` - This documentation

### Advanced Usage

For production use, consider:
- Enabling Link-Time Optimization (LTO) with `-flto`
- Using Clang static analyzer for code quality
- Integrating with LLVM sanitizers for debugging
- Exploring profile-guided optimization (PGO)

## Support

For issues specific to this Clang build setup:
1. Check the troubleshooting section above
2. Verify all paths in configuration files
3. Ensure LLVM was built with ARM support
4. Compare with working GCC build for reference

For general MIMXRT700 or MCUXpresso SDK issues, refer to NXP's documentation and support resources.
