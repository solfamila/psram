# MIMXRT700 XSPI PSRAM - LLVM/Clang Build Setup

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/your-username/mimxrt700-llvm-clang)
[![LLVM Version](https://img.shields.io/badge/LLVM-19.1.6-blue.svg)](https://llvm.org/)
[![MCU](https://img.shields.io/badge/MCU-MIMXRT798S-orange.svg)](https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/i-mx-rt-crossover-mcus/i-mx-rt700-crossover-mcu-with-arm-cortex-m33-and-dsp-cores:i.MX-RT700)

A complete LLVM/Clang 19.1.6 build setup for the MIMXRT700 XSPI PSRAM polling transfer project, demonstrating **44.5% binary size reduction** compared to ARM GCC while maintaining full MCUXpresso SDK compatibility.

## 🚀 Quick Start

```bash
# Clone this repository
git clone https://github.com/your-username/mimxrt700-llvm-clang.git
cd mimxrt700-llvm-clang

# Run the complete build (downloads LLVM, builds toolchain, compiles project)
chmod +x build_llvm_complete.sh
./build_llvm_complete.sh
```

## 📊 Performance Results

| Build Configuration | Binary Size | Flash Usage | Improvement |
|---------------------|-------------|-------------|-------------|
| **ARM GCC Release** | 35.7 KB | 1.76% | Baseline |
| **LLVM Clang Release** | **19.8 KB** | **0.98%** | **🚀 44.5% smaller** |

## 🎯 Key Features

- ✅ **Self-contained LLVM/Clang 19.1.6** build from source
- ✅ **ARM Cortex-M33** optimized compilation
- ✅ **Full MCUXpresso SDK** compatibility
- ✅ **Real debug console** support with PRINTF functionality
- ✅ **Hybrid toolchain** approach (Clang compilation + ARM GCC libraries)
- ✅ **Automated build process** with single script execution
- ✅ **44.5% binary size reduction** vs ARM GCC

## 📋 Prerequisites

### Required Software
- **CMake 3.15+**: `sudo apt install cmake`
- **Ninja Build System**: `sudo apt install ninja-build`
- **ARM GCC Toolchain**: For embedded libraries
- **wget and tar**: For downloading LLVM source
- **Git**: For cloning repositories

### System Requirements
- **8GB+ free disk space** (for LLVM build)
- **4GB+ RAM** (for parallel compilation)
- **Linux/macOS** (tested on Ubuntu 20.04+)
- **Internet connection** (for LLVM source download)

### Verify Prerequisites
```bash
cmake --version          # Should be 3.15+
ninja --version          # Any recent version
arm-none-eabi-gcc --version  # Should be available
```

## 🏗️ Project Structure

```
mimxrt700-llvm-clang/
├── README.md                           # This file
├── build_llvm_complete.sh              # Complete automated build script
├── cmake/
│   ├── CMakeLists_clang.txt           # Clang-specific CMake configuration
│   ├── clang_toolchain.cmake          # CMake toolchain file
│   ├── flags_clang.cmake              # Compiler and linker flags
│   └── config_clang.cmake             # SDK component configuration
├── docs/
│   ├── README_CLANG_BUILD.md          # Detailed build instructions
│   ├── REAL_DEBUG_CONSOLE_RESULTS.md  # Debug console implementation results
│   ├── LLVM_BUILD_RESULTS.md          # Comprehensive performance analysis
│   └── TROUBLESHOOTING.md             # Common issues and solutions
├── scripts/
│   ├── setup_environment.sh           # Environment setup helper
│   └── verify_build.sh                # Build verification script
└── examples/
    └── debug_console_usage.c          # Example debug console usage
```

## 🔧 Usage

### Option 1: Automated Build (Recommended)

```bash
# Complete setup - downloads LLVM, builds toolchain, compiles project
./build_llvm_complete.sh
```

This script will:
1. Download LLVM 19.1.6 source code (~500MB)
2. Build LLVM/Clang with ARM Cortex-M33 support (30-60 minutes)
3. Install toolchain locally in the workspace
4. Build both debug and release versions of the project
5. Generate comprehensive build reports

### Option 2: Manual Build

```bash
# 1. Set up environment
./scripts/setup_environment.sh

# 2. Download and build LLVM (if not already done)
wget -O llvm-project-19.1.6.tar.gz \
  https://github.com/llvm/llvm-project/archive/refs/tags/llvmorg-19.1.6.tar.gz
tar -xzf llvm-project-19.1.6.tar.gz
mv llvm-project-llvmorg-19.1.6 llvm-source

# 3. Build LLVM
mkdir -p llvm-build && cd llvm-build
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=$(pwd)/../llvm-install \
  -DLLVM_ENABLE_PROJECTS="clang;lld" \
  -DLLVM_TARGETS_TO_BUILD="ARM;AArch64;X86" \
  -DLLVM_DEFAULT_TARGET_TRIPLE="arm-none-eabi" \
  ../llvm-source/llvm
ninja -j4 && ninja install

# 4. Build project (requires MCUXpresso SDK project)
cd /path/to/your/mimxrt700_project/armgcc
mkdir build_clang && cd build_clang
cp /path/to/this/repo/cmake/* .
cmake -DCMAKE_TOOLCHAIN_FILE=clang_toolchain.cmake -DCMAKE_BUILD_TYPE=release .
make -j4
```

## 📖 Documentation

- **[Detailed Build Instructions](docs/README_CLANG_BUILD.md)** - Complete setup guide
- **[Debug Console Results](docs/REAL_DEBUG_CONSOLE_RESULTS.md)** - Real vs stub implementation
- **[Performance Analysis](docs/LLVM_BUILD_RESULTS.md)** - Comprehensive benchmarks
- **[Troubleshooting Guide](docs/TROUBLESHOOTING.md)** - Common issues and solutions

## 🔍 Build Verification

After building, verify your results:

```bash
# Run verification script
./scripts/verify_build.sh

# Manual verification
ls -la mimxrt700_project/armgcc/build_clang_release/release/
# Expected: xspi_psram_polling_transfer.bin (~20KB)
```

## 🛠️ Configuration Files

### Core Configuration Files

- **`cmake/CMakeLists_clang.txt`** - Project build configuration
- **`cmake/clang_toolchain.cmake`** - Toolchain setup
- **`cmake/flags_clang.cmake`** - Compiler/linker flags
- **`cmake/config_clang.cmake`** - SDK component selection

### Key Settings

```cmake
# Toolchain (clang_toolchain.cmake)
set(CMAKE_C_COMPILER "${LLVM_INSTALL_DIR}/bin/clang")
set(CMAKE_CXX_COMPILER "${LLVM_INSTALL_DIR}/bin/clang++")

# Debug Console (config_clang.cmake)
set(CONFIG_USE_utility_debug_console_lite true)
set(CONFIG_USE_utility_assert_lite true)
```

## 🐛 Troubleshooting

### Common Issues

1. **LLVM Build Fails**
   ```bash
   # Check disk space
   df -h
   # Ensure 8GB+ available
   ```

2. **Assembly Compilation Errors**
   ```bash
   # Verify toolchain uses Clang for assembly
   grep CMAKE_ASM_COMPILER cmake/clang_toolchain.cmake
   ```

3. **Missing ARM GCC**
   ```bash
   # Install ARM GCC toolchain
   sudo apt install gcc-arm-none-eabi
   ```

See [TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) for complete solutions.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/improvement`)
3. Commit your changes (`git commit -am 'Add improvement'`)
4. Push to the branch (`git push origin feature/improvement`)
5. Create a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **LLVM Project** - For the excellent compiler infrastructure
- **NXP Semiconductors** - For the MIMXRT700 MCU and MCUXpresso SDK
- **ARM** - For the Cortex-M33 architecture

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/your-username/mimxrt700-llvm-clang/issues)
- **Discussions**: [GitHub Discussions](https://github.com/your-username/mimxrt700-llvm-clang/discussions)
- **Documentation**: [Wiki](https://github.com/your-username/mimxrt700-llvm-clang/wiki)

---

**Note**: This repository contains build configuration and automation scripts only. You need to have the MIMXRT700 XSPI PSRAM project source code from the MCUXpresso SDK separately.
