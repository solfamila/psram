# Build Instructions

This document provides detailed build instructions for the MIMXRT700 XSPI PSRAM example project, including both local development and remote agent execution.

## Prerequisites

### Local Development Environment

#### Required Tools
- **ARM GCC Toolchain**: Version 10.3 or later
- **CMake**: Version 3.10 or later  
- **Ninja Build System**: For faster builds
- **Git**: For version control

#### Installing ARM GCC Toolchain

**Ubuntu/Debian:**
```bash
# Download and install ARM GCC
wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
sudo tar -xjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /opt/
export PATH="/opt/gcc-arm-none-eabi-10.3-2021.10/bin:$PATH"
```

**macOS:**
```bash
# Using Homebrew
brew install --cask gcc-arm-embedded
```

**Windows:**
```bash
# Download from ARM website and install
# Add to PATH: C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin
```

#### Installing Build Tools

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install cmake ninja-build build-essential git
```

**macOS:**
```bash
brew install cmake ninja git
```

**Windows:**
```bash
# Install via Visual Studio installer or chocolatey
choco install cmake ninja git
```

## Build Process

### 1. Environment Setup

Set required environment variables:
```bash
export ARMGCC_DIR="/opt/gcc-arm-none-eabi-10.3-2021.10"
export SDK_ROOT="$(pwd)/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__"
```

### 2. Navigate to Build Directory

```bash
cd mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc
```

### 3. Build Commands

#### Clean Previous Builds
```bash
./clean.sh
```

#### Debug Build
```bash
./build_debug.sh
```

#### Release Build  
```bash
./build_release.sh
```

#### Flash Debug Build
```bash
./build_flash_debug.sh
```

#### Flash Release Build
```bash
./build_flash_release.sh
```

### 4. Manual CMake Build

If you prefer manual CMake configuration:

```bash
# Create build directory
mkdir -p build_manual
cd build_manual

# Configure with CMake
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Build
ninja
```

## Build Configurations

### Debug Configuration
- **Optimization**: Disabled (-O0)
- **Debug Info**: Full debug symbols (-g3)
- **Memory**: RAM execution
- **Use Case**: Development and debugging

### Release Configuration  
- **Optimization**: Size optimized (-Os)
- **Debug Info**: Minimal
- **Memory**: RAM execution
- **Use Case**: Performance testing

### Flash Debug Configuration
- **Optimization**: Disabled (-O0)
- **Debug Info**: Full debug symbols (-g3)
- **Memory**: Flash execution
- **Use Case**: Flash-based debugging

### Flash Release Configuration
- **Optimization**: Size optimized (-Os)
- **Debug Info**: Minimal  
- **Memory**: Flash execution
- **Use Case**: Production deployment

## Build Artifacts

After successful build, artifacts are located in:
```
armgcc/debug/          # Debug builds
armgcc/release/        # Release builds
armgcc/flash_debug/    # Flash debug builds
armgcc/flash_release/  # Flash release builds
```

### Key Files
- `*.elf` - Executable file for debugging
- `*.bin` - Binary file for flashing
- `output.map` - Memory layout and symbol information

## Remote Agent Build

### Automatic Build Process

When using Augment's remote agent, builds are automatically handled:

1. **Environment Setup**: Container with ARM GCC toolchain
2. **Dependency Installation**: All required tools pre-installed
3. **Build Execution**: Automated build script execution
4. **Artifact Collection**: Build outputs collected and stored

### Remote Agent Configuration

The remote agent uses configuration from `.augment/config.yaml`:
- **Container**: Ubuntu 22.04 with ARM GCC
- **Resources**: 2 CPU cores, 4GB RAM, 10GB storage
- **Timeout**: 30 minutes maximum build time

### Triggering Remote Builds

Remote builds are triggered automatically when:
- Code changes are pushed to repository
- Manual build request through Augment interface
- Scheduled build execution

## Troubleshooting

### Common Issues

#### Toolchain Not Found
```
Error: arm-none-eabi-gcc: command not found
```
**Solution**: Ensure ARM GCC is installed and in PATH

#### CMake Version Too Old
```
Error: CMake 3.10 or higher is required
```
**Solution**: Update CMake to version 3.10+

#### Permission Denied on Scripts
```
Error: Permission denied: ./build_debug.sh
```
**Solution**: Make scripts executable
```bash
chmod +x *.sh
```

#### SDK Path Issues
```
Error: Could not find SDK files
```
**Solution**: Verify SDK_ROOT environment variable
```bash
export SDK_ROOT="$(pwd)/../__repo__"
```

### Build Verification

Verify successful build:
```bash
# Check if ELF file exists
ls -la debug/xspi_psram_polling_transfer_cm33_core0.elf

# Check file size (should be > 0)
file debug/xspi_psram_polling_transfer_cm33_core0.elf

# Verify ARM architecture
arm-none-eabi-objdump -f debug/xspi_psram_polling_transfer_cm33_core0.elf
```

### Performance Optimization

For faster builds:
1. **Use Ninja**: Already configured in build scripts
2. **Parallel Builds**: Ninja automatically uses multiple cores
3. **Incremental Builds**: Only rebuild changed files
4. **ccache**: Consider using ccache for repeated builds

## Advanced Build Options

### Custom CMake Options

```bash
cmake .. -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake \
  -DCUSTOM_OPTION=value
```

### Cross-Compilation Settings

The project uses ARM GCC cross-compilation:
- **Target**: ARM Cortex-M33
- **Architecture**: ARMv8-M
- **Float ABI**: Hard float
- **FPU**: FPv5-SP-D16

### Memory Configuration

Linker scripts define memory layout:
- **Flash**: Code and constants
- **RAM**: Variables and stack
- **PSRAM**: External memory testing

## Integration with IDEs

### VS Code
- Use CMake Tools extension
- Configure ARM GCC toolchain
- Set up debugging with J-Link

### IAR Embedded Workbench
- Project files available in `iar/` directory
- Import existing project configuration

### MCUXpresso IDE
- Project files available in `mdk/` directory
- Import as existing project
