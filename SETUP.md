# Environment Setup Guide

This guide helps you set up your development environment for the MIMXRT700 XSPI PSRAM example project, both for local development and remote agent compatibility.

## Overview

This project requires:
- Cross-compilation toolchain for ARM Cortex-M33
- Build system tools (CMake, Ninja)
- Hardware debugging tools (optional for local development)
- Container runtime (for remote agent simulation)

## Local Development Setup

### 1. Operating System Requirements

**Supported Platforms:**
- Ubuntu 20.04+ / Debian 11+
- macOS 11+ (Big Sur or later)
- Windows 10+ with WSL2 or native tools

### 2. Install ARM GCC Toolchain

#### Ubuntu/Debian
```bash
# Method 1: Download from ARM (Recommended)
cd /tmp
wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
sudo tar -xjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /opt/
sudo ln -s /opt/gcc-arm-none-eabi-10.3-2021.10 /opt/gcc-arm-none-eabi

# Method 2: Package manager (older version)
sudo apt-get install gcc-arm-none-eabi
```

#### macOS
```bash
# Using Homebrew
brew install --cask gcc-arm-embedded

# Or download from ARM website
# https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm
```

#### Windows
```bash
# Download installer from ARM website
# https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm

# Or using Chocolatey
choco install gcc-arm-embedded
```

### 3. Install Build Tools

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    python3 \
    python3-pip
```

#### macOS
```bash
# Install Xcode command line tools
xcode-select --install

# Install build tools
brew install cmake ninja git python3
```

#### Windows
```bash
# Using Chocolatey
choco install cmake ninja git python3

# Or download individual installers:
# CMake: https://cmake.org/download/
# Ninja: https://ninja-build.org/
# Git: https://git-scm.com/download/win
```

### 4. Environment Variables

Add to your shell profile (`~/.bashrc`, `~/.zshrc`, etc.):

```bash
# ARM GCC Toolchain
export ARMGCC_DIR="/opt/gcc-arm-none-eabi"
export PATH="$ARMGCC_DIR/bin:$PATH"

# Project-specific
export SDK_ROOT="$HOME/path/to/project/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__"
```

Reload your shell:
```bash
source ~/.bashrc  # or ~/.zshrc
```

### 5. Verify Installation

```bash
# Check ARM GCC
arm-none-eabi-gcc --version
arm-none-eabi-gdb --version

# Check build tools
cmake --version
ninja --version
git --version

# Check Python (for testing framework)
python3 --version
```

Expected output should show:
- ARM GCC version 10.3 or later
- CMake version 3.10 or later
- Ninja version 1.8 or later

## Hardware Setup (Optional)

### 1. MIMXRT700-EVK Board

**Required Hardware:**
- MIMXRT700-EVK evaluation board
- USB-C cable for power and debugging
- J-Link debugger (or compatible SWD debugger)

**Board Configuration:**
- Ensure XSPI PSRAM is properly connected
- Set jumpers for desired boot mode
- Connect debugger to SWD interface

### 2. Debugger Setup

#### J-Link
```bash
# Download J-Link software
# https://www.segger.com/downloads/jlink/

# Ubuntu/Debian
sudo dpkg -i JLink_Linux_x86_64.deb

# macOS
# Install .pkg file from SEGGER

# Windows  
# Run installer from SEGGER
```

#### OpenOCD (Alternative)
```bash
# Ubuntu/Debian
sudo apt-get install openocd

# macOS
brew install openocd

# Windows
# Download from OpenOCD website
```

## Remote Agent Environment

### 1. Docker Setup (For Local Testing)

#### Install Docker
```bash
# Ubuntu/Debian
sudo apt-get install docker.io docker-compose
sudo usermod -aG docker $USER

# macOS
brew install --cask docker

# Windows
# Download Docker Desktop from docker.com
```

#### Test Remote Agent Environment
```bash
# Build the container
cd ex
docker build -f .augment/Dockerfile -t mimxrt700-dev .

# Run interactive container
docker run -it --rm -v $(pwd):/workspace mimxrt700-dev

# Inside container, test build
cd /workspace/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc
./build_debug.sh
```

### 2. Remote Agent Configuration

The project includes pre-configured files:
- `.augment/config.yaml` - Agent settings
- `.augment/Dockerfile` - Build environment
- `.gitignore` - Artifact exclusion

No additional configuration needed for basic remote agent usage.

## IDE Integration

### 1. Visual Studio Code

#### Install Extensions
```bash
# Install VS Code extensions
code --install-extension ms-vscode.cpptools
code --install-extension ms-vscode.cmake-tools
code --install-extension marus25.cortex-debug
```

#### Configure Workspace
```json
// .vscode/settings.json
{
    "cmake.configureArgs": [
        "-DCMAKE_TOOLCHAIN_FILE=${workspaceFolder}/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc/toolchain.cmake"
    ],
    "C_Cpp.default.compilerPath": "/opt/gcc-arm-none-eabi/bin/arm-none-eabi-gcc"
}
```

### 2. CLion

#### Configure Toolchain
1. File → Settings → Build, Execution, Deployment → Toolchains
2. Add new toolchain: "ARM GCC"
3. Set C Compiler: `/opt/gcc-arm-none-eabi/bin/arm-none-eabi-gcc`
4. Set C++ Compiler: `/opt/gcc-arm-none-eabi/bin/arm-none-eabi-g++`

## Testing Framework Setup

### 1. Unity Testing Framework

```bash
# Clone Unity framework
git clone https://github.com/ThrowTheSwitch/Unity.git /opt/unity

# Add to environment
export UNITY_ROOT="/opt/unity"
```

### 2. Create Test Directory Structure

```bash
mkdir -p ex/tests
mkdir -p ex/tests/unit
mkdir -p ex/tests/integration
```

## Troubleshooting

### Common Issues

#### Permission Denied
```bash
# Make scripts executable
chmod +x mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc/*.sh
```

#### Toolchain Not Found
```bash
# Verify PATH
echo $PATH | grep arm-none-eabi

# Check installation
which arm-none-eabi-gcc
```

#### CMake Configuration Failed
```bash
# Clear CMake cache
rm -rf CMakeCache.txt CMakeFiles/

# Reconfigure
cmake .. -G Ninja
```

### Environment Validation Script

Create a validation script:

```bash
#!/bin/bash
# validate_environment.sh

echo "Validating development environment..."

# Check ARM GCC
if command -v arm-none-eabi-gcc &> /dev/null; then
    echo "✓ ARM GCC found: $(arm-none-eabi-gcc --version | head -n1)"
else
    echo "✗ ARM GCC not found"
    exit 1
fi

# Check CMake
if command -v cmake &> /dev/null; then
    echo "✓ CMake found: $(cmake --version | head -n1)"
else
    echo "✗ CMake not found"
    exit 1
fi

# Check Ninja
if command -v ninja &> /dev/null; then
    echo "✓ Ninja found: $(ninja --version)"
else
    echo "✗ Ninja not found"
    exit 1
fi

echo "Environment validation complete!"
```

## Next Steps

After completing setup:

1. **Test Build**: Run `./build_debug.sh` to verify everything works
2. **Read Documentation**: Review README.md and BUILD.md
3. **Explore Code**: Examine the XSPI PSRAM example implementation
4. **Set Up Testing**: Implement unit tests using Unity framework
5. **Configure CI/CD**: Set up automated builds and testing

## Support Resources

- **NXP Documentation**: SDK and hardware manuals
- **ARM GCC Documentation**: Toolchain reference
- **CMake Documentation**: Build system reference
- **Augment Support**: Remote agent assistance
