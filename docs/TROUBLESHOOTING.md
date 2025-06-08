# Troubleshooting Guide - MIMXRT700 LLVM/Clang Build

This guide covers common issues and solutions when building the MIMXRT700 XSPI PSRAM project with LLVM/Clang.

## Prerequisites Issues

### Issue: CMake Version Too Old

**Problem:**
```
CMake Error: CMake 3.15 or higher is required. You are running version 3.10.2
```

**Solution:**
```bash
# Ubuntu 18.04/20.04
sudo apt remove cmake
sudo snap install cmake --classic

# Or build from source
wget https://github.com/Kitware/CMake/releases/download/v3.25.0/cmake-3.25.0.tar.gz
tar -xzf cmake-3.25.0.tar.gz
cd cmake-3.25.0
./bootstrap && make -j4 && sudo make install
```

### Issue: ARM GCC Not Found

**Problem:**
```
arm-none-eabi-gcc: command not found
```

**Solution:**
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install gcc-arm-none-eabi

# Verify installation
arm-none-eabi-gcc --version
```

### Issue: Ninja Not Found

**Problem:**
```
ninja: command not found
```

**Solution:**
```bash
# Ubuntu/Debian
sudo apt install ninja-build

# Verify installation
ninja --version
```

## LLVM Build Issues

### Issue: Insufficient Disk Space

**Problem:**
```
No space left on device
```

**Solution:**
```bash
# Check available space
df -h

# Clean up if needed
sudo apt autoremove
sudo apt autoclean

# Move build to different partition if needed
export WORKSPACE_DIR=/path/to/larger/partition
```

### Issue: LLVM Build Fails with Memory Error

**Problem:**
```
c++: fatal error: Killed signal terminated program cc1plus
```

**Solution:**
```bash
# Reduce parallel jobs
ninja -j2  # Instead of -j4

# Or add swap space
sudo fallocate -l 4G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
```

### Issue: LLVM Download Fails

**Problem:**
```
wget: unable to resolve host address 'github.com'
```

**Solution:**
```bash
# Check internet connection
ping github.com

# Try alternative download
curl -L -o llvm-project-19.1.6.tar.gz \
  https://github.com/llvm/llvm-project/archive/refs/tags/llvmorg-19.1.6.tar.gz

# Or download manually and place in workspace
```

## Project Build Issues

### Issue: Assembly Compilation Errors

**Problem:**
```
arm-none-eabi-gcc: error: unrecognized command-line option '-target'
```

**Solution:**
Ensure `clang_toolchain.cmake` uses Clang for assembly:
```cmake
set(CMAKE_ASM_COMPILER "${LLVM_INSTALL_DIR}/bin/clang")
```

### Issue: Linker Errors with _start Symbol

**Problem:**
```
ld.lld: error: undefined symbol: _start
```

**Solution:**
Check `flags_clang.cmake` includes symbol definitions:
```cmake
-Wl,--entry=Reset_Handler
-Wl,--defsym=_start=main
-Wl,--defsym=__main=main
```

### Issue: Missing Debug Console Components

**Problem:**
Build succeeds but PRINTF doesn't work

**Solution:**
Verify `config_clang.cmake` enables debug console:
```cmake
set(CONFIG_USE_utility_debug_console_lite true)
set(CONFIG_USE_utility_assert_lite true)
set(CONFIG_USE_utility_str true)
set(CONFIG_USE_component_lpuart_adapter true)
```

### Issue: CMake Configuration Fails

**Problem:**
```
CMake Error: Could not find CMAKE_C_COMPILER
```

**Solution:**
```bash
# Verify LLVM installation
ls -la llvm-install/bin/clang

# Check environment variables
echo $LLVM_INSTALL_DIR

# Rebuild LLVM if necessary
rm -rf llvm-build llvm-install
./build_llvm_complete.sh
```

### Issue: Project Directory Not Found

**Problem:**
```
Error: Project directory not found: /path/to/project
```

**Solution:**
```bash
# Set correct project path
export PROJECT_DIR=/path/to/your/mimxrt700_project

# Or pass as argument
./build_llvm_complete.sh /path/to/your/mimxrt700_project

# Verify project structure
ls -la $PROJECT_DIR/armgcc/
```

## Runtime Issues

### Issue: Binary Too Large

**Problem:**
Binary size larger than expected

**Solution:**
```bash
# Check build type
grep CMAKE_BUILD_TYPE CMakeCache.txt

# Ensure release build
cmake -DCMAKE_BUILD_TYPE=release .

# Check optimization flags
grep "\-O" flags_clang.cmake
```

### Issue: PRINTF Not Working

**Problem:**
No output from PRINTF statements

**Solution:**
```bash
# Verify debug console symbols
llvm-objdump -t binary.elf | grep -i printf

# Check UART configuration in board.c
# Verify baud rate and pin configuration

# Test with simple output
PRINTF("Test\r\n");
```

### Issue: Hard Fault on Startup

**Problem:**
MCU crashes immediately after reset

**Solution:**
```bash
# Check memory layout
llvm-objdump -h binary.elf

# Verify linker script
grep MEMORY *.ld

# Check stack/heap sizes
grep -E "(stack|heap)" *.ld
```

## Performance Issues

### Issue: Slow Build Times

**Problem:**
Build takes too long

**Solution:**
```bash
# Use more CPU cores (if available)
make -j$(nproc)

# Use ccache for faster rebuilds
sudo apt install ccache
export CC="ccache clang"
export CXX="ccache clang++"

# Clean unnecessary files
make clean
```

### Issue: Large Debug Build

**Problem:**
Debug build is very large

**Solution:**
```bash
# This is normal for debug builds
# Use release build for size optimization
cmake -DCMAKE_BUILD_TYPE=release .

# Strip debug symbols if needed
llvm-strip binary.elf
```

## Environment Issues

### Issue: Path Problems

**Problem:**
Tools not found in PATH

**Solution:**
```bash
# Add LLVM to PATH
export PATH=$PWD/llvm-install/bin:$PATH

# Add to .bashrc for persistence
echo 'export PATH=$HOME/workspace/llvm-install/bin:$PATH' >> ~/.bashrc
```

### Issue: Permission Denied

**Problem:**
```
Permission denied: ./build_llvm_complete.sh
```

**Solution:**
```bash
# Make script executable
chmod +x build_llvm_complete.sh
chmod +x scripts/*.sh

# Check file permissions
ls -la *.sh
```

## Advanced Debugging

### Enable Verbose Output

```bash
# CMake verbose
cmake -DCMAKE_VERBOSE_MAKEFILE=ON .

# Make verbose
make VERBOSE=1

# Ninja verbose
ninja -v
```

### Check Build Dependencies

```bash
# List linked libraries
llvm-objdump -p binary.elf | grep NEEDED

# Check symbol dependencies
llvm-nm binary.elf | grep -E "(U|T)"

# Analyze binary sections
llvm-readelf -S binary.elf
```

### Memory Analysis

```bash
# Detailed memory usage
llvm-size -A binary.elf

# Memory map analysis
cat output.map | grep -A 20 "Memory Configuration"

# Stack usage analysis
llvm-objdump -d binary.elf | grep -E "(push|pop|sp)"
```

## Getting Help

If you encounter issues not covered here:

1. **Check the build log** for specific error messages
2. **Verify prerequisites** with `./scripts/setup_environment.sh`
3. **Run verification** with `./scripts/verify_build.sh`
4. **Search GitHub issues** for similar problems
5. **Create a new issue** with:
   - Complete error message
   - Build environment details
   - Steps to reproduce

## Common Error Patterns

| Error Pattern | Likely Cause | Quick Fix |
|---------------|--------------|-----------|
| `command not found` | Missing prerequisite | Install missing tool |
| `No space left` | Insufficient disk space | Free up space or change location |
| `Permission denied` | File permissions | `chmod +x` script files |
| `undefined symbol` | Linker configuration | Check flags_clang.cmake |
| `unrecognized option` | Wrong compiler | Check toolchain configuration |
| `CMake Error` | Configuration issue | Clean and reconfigure |

Remember: Most issues are related to prerequisites, paths, or configuration. Double-check these first before diving into complex debugging.
