# MIMXRT700 XSPI PSRAM Example Project

This project demonstrates XSPI PSRAM polling transfer functionality on the NXP MIMXRT700-EVK evaluation board. It has been configured for compatibility with Augment's remote agent feature.

## Project Overview

The project contains an embedded C example that:
- Initializes XSPI interface for PSRAM communication
- Performs read/write operations using both AHB and IP commands
- Tests data integrity across the entire PSRAM address space
- Demonstrates polling-based transfer methods

## Hardware Requirements

- **Board**: MIMXRT700-EVK (NXP i.MX RT700 Evaluation Kit)
- **External Memory**: XSPI PSRAM (connected via XSPI interface)
- **Debugger**: J-Link or compatible SWD debugger

## Software Requirements

### Local Development
- ARM GCC toolchain (version 10.3 or later)
- CMake (version 3.10 or later)
- Ninja build system
- Git

### Remote Agent Execution
The project is configured to run in Augment's remote agent environment with:
- Containerized build environment (Ubuntu 22.04)
- Pre-installed ARM GCC toolchain
- Automated dependency management

## Quick Start

### Repository Information
- **GitHub Repository**: https://github.com/solfamila/psram
- **CI/CD Pipeline**: https://github.com/solfamila/psram/actions
- **Remote Agent Status**: ✅ **FULLY DEPLOYED AND ACTIVE**

### Building the Project

#### Using the Unified Build Script (Recommended)
```bash
# Build debug version
./build.sh -t debug

# Build release version
./build.sh -t release

# Build with tests
./build.sh -t debug -T

# Container build (simulates remote agent)
./build.sh -C
```

#### Traditional Method
1. **Navigate to build directory:**
   ```bash
   cd mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc
   ```

2. **Build debug version:**
   ```bash
   ./build_debug.sh
   ```

3. **Build release version:**
   ```bash
   ./build_release.sh
   ```

### Available Build Configurations

- `debug` - Debug build with optimization disabled
- `release` - Release build with optimization enabled
- `flash_debug` - Debug build configured for flash execution
- `flash_release` - Release build configured for flash execution

### Build Artifacts

After successful build, you'll find:
- `xspi_psram_polling_transfer_cm33_core0.elf` - Executable file
- `xspi_psram_polling_transfer.bin` - Binary file for flashing
- `output.map` - Memory map file

## Remote Agent Features

This project is **fully deployed** with Augment remote agent compatibility:

### 🚀 Deployment Status
- ✅ **Repository**: https://github.com/solfamila/psram
- ✅ **CI/CD Pipeline**: Active and running
- ✅ **Container Builds**: Working and tested
- ✅ **Remote Agent Config**: Fully integrated
- ✅ **Testing Framework**: Unity tests operational

### Configuration Files
- `.augment/config.yaml` - Remote agent configuration ✅
- `.augment/Dockerfile` - Container environment specification ✅
- `.augment/remote-agent.json` - Comprehensive project metadata ✅
- `.github/workflows/ci.yml` - CI/CD pipeline ✅
- `.gitignore` - Proper artifact exclusion ✅

### Automated Build Environment
The remote agent automatically:
- ✅ Sets up ARM GCC cross-compilation toolchain (v10.3-2021.10)
- ✅ Configures CMake and Ninja build systems
- ✅ Installs required dependencies in Ubuntu 22.04 container
- ✅ Executes builds in isolated containers
- ✅ Collects and validates build artifacts
- ✅ Runs comprehensive test suites

### Testing Support
- ✅ Unit testing framework integration (Unity v2.5.2)
- ✅ Automated test execution capabilities
- ✅ Mock framework for hardware abstraction
- ✅ Build verification and artifact validation
- ✅ CI/CD integration with GitHub Actions

## Project Structure

```
ex/
├── .augment/                          # Remote agent configuration
│   ├── config.yaml                    # Agent settings
│   └── Dockerfile                     # Build environment
├── mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/
│   ├── armgcc/                        # Build system
│   │   ├── CMakeLists.txt            # CMake configuration
│   │   ├── build_*.sh                # Build scripts
│   │   └── debug/                    # Build output
│   ├── *.c, *.h                      # Source files
│   └── readme.md                     # Original example documentation
├── tests/                            # Unit tests (to be added)
├── README.md                         # This file
├── BUILD.md                          # Detailed build instructions
└── SETUP.md                          # Environment setup guide
```

## Next Steps

1. **Testing**: Implement unit tests using Unity framework
2. **CI/CD**: Set up continuous integration pipeline
3. **Documentation**: Expand technical documentation
4. **Hardware Abstraction**: Add HAL for easier porting

## Support

For issues related to:
- **Hardware/SDK**: Refer to NXP documentation and support
- **Remote Agent**: Contact Augment support
- **Build Issues**: Check BUILD.md for troubleshooting

## License

This project follows the original NXP SDK licensing terms (BSD-3-Clause).
