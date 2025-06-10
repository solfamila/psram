# Quick LLVM/Clang Build Guide

## TL;DR - Fast Build Instructions

### Prerequisites (Ubuntu/Debian)
```bash
sudo apt install gcc-arm-none-eabi cmake ninja-build build-essential
```

### Build LLVM + Compile Project (One Command)
```bash
./build_llvm_complete.sh /home/smw016108/Downloads/ex/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0
```

## What This Does
1. **Builds LLVM/Clang 19.1.6** with ARM support (if not already built)
2. **Compiles the PSRAM project** using the hybrid toolchain:
   - LLVM/Clang for C compilation
   - System ARM GCC for libraries and linking

## Key Working Configuration

### System Paths (WORKING ✅)
- **ARM GCC Sysroot**: `/usr/lib/arm-none-eabi` 
- **GCC Libraries**: `/usr/lib/gcc/arm-none-eabi/13.2.1/`
- **LLVM Install**: `/home/smw016108/Downloads/ex/llvm-install/`

### DON'T Use These Paths (BROKEN ❌)
- `/opt/gcc-arm-none-eabi-*` - Doesn't exist on most systems
- Manual ARM GCC downloads - Use system package instead

## Alternative: Direct Compilation (Fastest)

If you just want to compile source files to LLVM IR:
```bash
./compile_all_drivers_to_llvm.sh
```

This uses the proven working flags and compiles all drivers to LLVM IR files.

## Troubleshooting

### "assert.h not found" Error
- **Cause**: Wrong sysroot path in cmake/clang_toolchain.cmake
- **Fix**: Ensure it uses `/usr/lib/arm-none-eabi` not `/opt/` paths

### "arm-none-eabi-gcc not found"
```bash
sudo apt install gcc-arm-none-eabi
```

### CMake Configuration Fails
- Check that `cmake/clang_toolchain.cmake` has the correct system paths
- The file has been fixed with working configuration and comments

## Files Generated
- **LLVM IR**: `clang_ir_final/drivers/*.ll` and `clang_ir_final/main_project/*.ll`
- **Object Files**: `clang_build_final/*.o`
- **Executables**: `build_clang_debug/debug/*.elf`

## Time Estimates
- **LLVM Build**: 30-60 minutes (one-time)
- **Project Compilation**: 1-2 minutes
- **IR Generation**: 30 seconds

---
**Note**: This configuration is tested and working. Don't overthink it - just run the scripts!
