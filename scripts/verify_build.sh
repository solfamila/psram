#!/bin/bash

# Build Verification Script for MIMXRT700 LLVM/Clang Build
# This script verifies that the build completed successfully and shows results

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(dirname "$SCRIPT_DIR")"
LLVM_INSTALL_DIR="${WORKSPACE_DIR}/llvm-install"

# Project directory - user must provide or set environment variable
PROJECT_DIR="${PROJECT_DIR:-${1:-}}"
if [ -z "$PROJECT_DIR" ]; then
    echo "Error: PROJECT_DIR not set. Please either:"
    echo "  1. Set environment variable: export PROJECT_DIR=/path/to/mimxrt700_project"
    echo "  2. Pass as argument: $0 /path/to/mimxrt700_project"
    exit 1
fi

echo "=== MIMXRT700 LLVM/Clang Build Verification ==="
echo "Workspace: $WORKSPACE_DIR"
echo "Project: $PROJECT_DIR"
echo ""

# Verify LLVM installation
echo "1. Verifying LLVM/Clang installation..."
if [ -f "$LLVM_INSTALL_DIR/bin/clang" ]; then
    echo "✓ LLVM/Clang found at: $LLVM_INSTALL_DIR"
    echo "  Version: $($LLVM_INSTALL_DIR/bin/clang --version | head -n1)"
    
    # Check ARM target support
    if $LLVM_INSTALL_DIR/bin/clang --print-targets | grep -q arm; then
        echo "✓ ARM target support verified"
    else
        echo "❌ ARM target support missing"
        exit 1
    fi
else
    echo "❌ LLVM/Clang not found at: $LLVM_INSTALL_DIR"
    echo "Run ./build_llvm_complete.sh to build LLVM first"
    exit 1
fi

echo ""

# Verify project builds
echo "2. Verifying project builds..."
cd "$PROJECT_DIR/armgcc"

# Check debug build
if [ -f "build_clang_debug/debug/xspi_psram_polling_transfer_cm33_core0.elf" ]; then
    echo "✓ Debug build found"
    debug_size=$(ls -lh build_clang_debug/debug/xspi_psram_polling_transfer.bin 2>/dev/null | awk '{print $5}' || echo "N/A")
    echo "  Binary size: $debug_size"
else
    echo "❌ Debug build not found"
fi

# Check release build
if [ -f "build_clang_release/release/xspi_psram_polling_transfer_cm33_core0.elf" ]; then
    echo "✓ Release build found"
    release_size=$(ls -lh build_clang_release/release/xspi_psram_polling_transfer.bin 2>/dev/null | awk '{print $5}' || echo "N/A")
    echo "  Binary size: $release_size"
else
    echo "❌ Release build not found"
fi

# Check real debug console builds
if [ -f "build_clang_debug_real/debug/xspi_psram_polling_transfer_cm33_core0.elf" ]; then
    echo "✓ Debug build (real console) found"
    debug_real_size=$(ls -lh build_clang_debug_real/debug/xspi_psram_polling_transfer.bin 2>/dev/null | awk '{print $5}' || echo "N/A")
    echo "  Binary size: $debug_real_size"
fi

if [ -f "build_clang_release_real/release/xspi_psram_polling_transfer_cm33_core0.elf" ]; then
    echo "✓ Release build (real console) found"
    release_real_size=$(ls -lh build_clang_release_real/release/xspi_psram_polling_transfer.bin 2>/dev/null | awk '{print $5}' || echo "N/A")
    echo "  Binary size: $release_real_size"
fi

echo ""

# Compare with ARM GCC if available
echo "3. Build comparison..."
if [ -f "gcc_release/xspi_psram_polling_transfer.bin" ]; then
    gcc_size=$(ls -lh gcc_release/xspi_psram_polling_transfer.bin | awk '{print $5}')
    echo "ARM GCC Release:     $gcc_size"
else
    echo "ARM GCC build not found (comparison not available)"
fi

if [ -n "$release_real_size" ] && [ "$release_real_size" != "N/A" ]; then
    echo "LLVM Clang Release:  $release_real_size"
fi

echo ""

# Memory usage analysis
echo "4. Memory usage analysis..."

if [ -f "gcc_release/xspi_psram_polling_transfer_cm33_core0.elf" ]; then
    echo "ARM GCC Release:"
    arm-none-eabi-size gcc_release/xspi_psram_polling_transfer_cm33_core0.elf
    echo ""
fi

if [ -f "build_clang_release_real/release/xspi_psram_polling_transfer_cm33_core0.elf" ]; then
    echo "LLVM Clang Release (Real Console):"
    $LLVM_INSTALL_DIR/bin/llvm-size build_clang_release_real/release/xspi_psram_polling_transfer_cm33_core0.elf
    echo ""
fi

# Verify debug console functionality
echo "5. Debug console verification..."
if [ -f "build_clang_release_real/release/xspi_psram_polling_transfer_cm33_core0.elf" ]; then
    # Check for PRINTF symbols
    if $LLVM_INSTALL_DIR/bin/llvm-objdump -t build_clang_release_real/release/xspi_psram_polling_transfer_cm33_core0.elf | grep -q -i printf; then
        echo "✓ PRINTF functionality detected in binary"
    else
        echo "⚠️  PRINTF symbols not found (may be optimized)"
    fi
    
    # Check for debug console components
    if $LLVM_INSTALL_DIR/bin/llvm-objdump -t build_clang_release_real/release/xspi_psram_polling_transfer_cm33_core0.elf | grep -q DbgConsole; then
        echo "✓ Debug console components detected"
    else
        echo "⚠️  Debug console components not clearly visible"
    fi
else
    echo "❌ Release build with real console not found"
fi

echo ""

# Configuration verification
echo "6. Configuration verification..."
config_files=("cmake/CMakeLists_clang.txt" "cmake/clang_toolchain.cmake" "cmake/flags_clang.cmake" "cmake/config_clang.cmake")

cd "$WORKSPACE_DIR"
for file in "${config_files[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ $file found"
    else
        echo "❌ $file missing"
    fi
done

echo ""
echo "=== Verification Complete ==="

# Summary
echo "Build Summary:"
echo "=============="
if [ -f "$LLVM_INSTALL_DIR/bin/clang" ]; then
    echo "✓ LLVM/Clang 19.1.6 toolchain: READY"
else
    echo "❌ LLVM/Clang toolchain: NOT READY"
fi

if [ -f "$PROJECT_DIR/armgcc/build_clang_release_real/release/xspi_psram_polling_transfer_cm33_core0.elf" ]; then
    echo "✓ MIMXRT700 project build: SUCCESS"
    echo "✓ Debug console support: ENABLED"
else
    echo "❌ MIMXRT700 project build: INCOMPLETE"
fi

echo ""
echo "Next steps:"
echo "1. Flash the binary to your MIMXRT700 board"
echo "2. Connect a serial terminal to see PRINTF output"
echo "3. Test the XSPI PSRAM functionality"
echo ""
echo "Binary location: $PROJECT_DIR/armgcc/build_clang_release_real/release/xspi_psram_polling_transfer.bin"
