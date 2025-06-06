# Environment Setup Guide

This guide helps you set up your development environment for the MIMXRT700 XSPI PSRAM example project, both for local development and remote agent compatibility.

## Overview

This project requires:
- **NXP MIMXRT700 SDK** (version 25.03.00 or later) - **INCLUDED IN REPOSITORY**
- Cross-compilation toolchain for ARM Cortex-M33 (GCC ARM Embedded 10.3-2021.10 or later)
- Build system tools (CMake 3.10+, Ninja)
- Hardware debugging tools (optional for local development)
- Container runtime (for remote agent simulation)

## ⚠️ IMPORTANT: SDK Information

### SDK Status: ✅ **INCLUDED AND READY**
The NXP MIMXRT700 SDK is **already included** in this repository under:
```
mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/
```

### SDK Details:
- **Version**: 25.03.00 (MCUXpresso SDK)
- **License**: BSD-3-Clause (allows redistribution)
- **Size**: ~500MB (complete SDK with all components)
- **Components**: CMSIS, device drivers, board support, middleware

### SDK Validation:
The build system automatically validates the SDK installation. You can manually verify by checking:
```bash
# Check SDK structure
ls mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/
# Should show: CMSIS, devices, boards, components, etc.

# Check SDK version
grep 'version=' mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/MIMXRT700-EVK_manifest_v3_15.xml
```

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

### Common Issues and Solutions

#### 1. SDK-Related Issues

**Problem**: "SDK directory not found" or "Required SDK directory missing"
```bash
# Solution 1: Verify repository completeness
git status
git submodule update --init --recursive

# Solution 2: Check SDK structure
ls -la mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/
# Should show: CMSIS, devices, boards, components, etc.

# Solution 3: Set SDK_ROOT manually
export SDK_ROOT="$(pwd)/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__"
```

**Problem**: "SDK version may be incompatible"
```bash
# Check current SDK version
grep 'version=' mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/MIMXRT700-EVK_manifest_v3_15.xml

# Expected: version="25.03.00" or later
# If older version found, update the repository or download latest SDK
```

#### 2. Toolchain Issues

**Problem**: "ARM GCC toolchain not found"
```bash
# Solution 1: Install ARM GCC
# Ubuntu/Debian:
sudo apt-get install gcc-arm-none-eabi

# Or download from ARM:
wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
sudo tar -xjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /opt/
export ARMGCC_DIR="/opt/gcc-arm-none-eabi-10.3-2021.10"
export PATH="$ARMGCC_DIR/bin:$PATH"

# Solution 2: Verify installation
which arm-none-eabi-gcc
arm-none-eabi-gcc --version
```

**Problem**: "arm-none-eabi-gcc not found in PATH"
```bash
# Add to PATH
export PATH="/opt/gcc-arm-none-eabi/bin:$PATH"

# Make permanent (add to ~/.bashrc or ~/.zshrc)
echo 'export PATH="/opt/gcc-arm-none-eabi/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

#### 3. Build System Issues

**Problem**: Permission denied on build scripts
```bash
# Make scripts executable
chmod +x mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc/*.sh
chmod +x build.sh
```

**Problem**: CMake configuration failed
```bash
# Clear CMake cache
cd mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc
rm -rf CMakeCache.txt CMakeFiles/ debug/ release/

# Verify environment
export ARMGCC_DIR="/opt/gcc-arm-none-eabi"  # or your toolchain path
export SDK_ROOT="$(pwd)/../__repo__"

# Reconfigure
cmake . -G Ninja
```

**Problem**: "No rule to make target" or missing files
```bash
# Verify SDK completeness
ls -la ../__repo__/devices/MIMXRT798S/
ls -la ../__repo__/boards/mimxrt700evk/

# If files missing, re-clone repository
git clone --recursive https://github.com/solfamila/psram.git
```

#### 4. Container/Docker Issues

**Problem**: Docker build fails
```bash
# Check Docker installation
docker --version

# Build with verbose output
docker build -f .augment/Dockerfile -t mimxrt700-dev . --progress=plain

# Check for disk space
df -h
```

**Problem**: Container build succeeds but runtime fails
```bash
# Run container interactively for debugging
docker run -it --rm -v $(pwd):/workspace mimxrt700-dev /bin/bash

# Inside container, validate environment
validate-sdk
arm-none-eabi-gcc --version
```

#### 5. Environment-Specific Issues

**Problem**: macOS build issues
```bash
# Install Xcode command line tools
xcode-select --install

# Use Homebrew for dependencies
brew install cmake ninja
brew install --cask gcc-arm-embedded
```

**Problem**: Windows/WSL issues
```bash
# Ensure WSL2 is used
wsl --set-version Ubuntu 2

# Install dependencies in WSL
sudo apt-get update
sudo apt-get install build-essential cmake ninja-build
```

#### 6. Testing Issues

**Problem**: Unity tests fail to build
```bash
# Install Unity framework
git clone https://github.com/ThrowTheSwitch/Unity.git ~/unity
export UNITY_ROOT="$HOME/unity"

# Or use system package
sudo apt-get install libunity-dev
```

**Problem**: Tests compile but fail to run
```bash
# Check test executable
ls -la tests/build/run_tests
file tests/build/run_tests

# Run with verbose output
cd tests/build
./run_tests -v
```

### Getting Help

If you encounter issues not covered here:

1. **Check the build log**: Run with `-v` flag for verbose output
2. **Validate environment**: Use the validation script above
3. **Check GitHub Issues**: https://github.com/solfamila/psram/issues
4. **Container debugging**: Use the Docker container for a clean environment

### Environment Reset

If all else fails, reset your environment:
```bash
# Clean all build artifacts
./build.sh -c

# Reset environment variables
unset ARMGCC_DIR SDK_ROOT UNITY_ROOT

# Re-run setup
./build.sh --help  # This validates and sets up environment
```

### Environment Validation Script

The project includes a comprehensive validation script in `build.sh`. You can also create a standalone validation script:

```bash
#!/bin/bash
# validate_environment.sh

echo "=== MIMXRT700 Development Environment Validation ==="

# Check ARM GCC
if command -v arm-none-eabi-gcc &> /dev/null; then
    GCC_VERSION=$(arm-none-eabi-gcc --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+' | head -n1)
    echo "✓ ARM GCC found: $GCC_VERSION"
    if [[ "$GCC_VERSION" < "10.3.0" ]]; then
        echo "⚠️  Warning: ARM GCC version may be incompatible. Recommended: 10.3.0+"
    fi
else
    echo "✗ ARM GCC not found"
    echo "  Install from: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm"
    exit 1
fi

# Check CMake
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+')
    echo "✓ CMake found: $CMAKE_VERSION"
    if [[ "$CMAKE_VERSION" < "3.10.0" ]]; then
        echo "✗ CMake version too old. Required: 3.10.0+"
        exit 1
    fi
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

# Check SDK
SDK_ROOT="${SDK_ROOT:-$(pwd)/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__}"
if [[ -d "$SDK_ROOT" ]]; then
    echo "✓ NXP SDK found at: $SDK_ROOT"

    # Check essential SDK components
    for dir in CMSIS devices boards components; do
        if [[ -d "$SDK_ROOT/$dir" ]]; then
            echo "  ✓ $dir directory found"
        else
            echo "  ✗ $dir directory missing"
            exit 1
        fi
    done

    # Check SDK version
    if [[ -f "$SDK_ROOT/MIMXRT700-EVK_manifest_v3_15.xml" ]]; then
        SDK_VERSION=$(grep -o 'version="[^"]*"' "$SDK_ROOT/MIMXRT700-EVK_manifest_v3_15.xml" | head -n1 | cut -d'"' -f2)
        echo "  ✓ SDK Version: $SDK_VERSION"
    fi
else
    echo "✗ NXP SDK not found at: $SDK_ROOT"
    echo "  Please ensure the repository is complete or set SDK_ROOT environment variable"
    exit 1
fi

echo "✅ Environment validation complete! Ready to build."
```

### Quick Validation
Run the built-in validation:
```bash
./build.sh --help  # This will validate the environment
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
