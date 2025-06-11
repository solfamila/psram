#!/bin/bash
# Simple Clang build script for MIMXRT700 XSPI PSRAM Polling Transfer
# This script builds the project using system-installed Clang

set -e  # Exit on any error

echo "🔨 Building MIMXRT700 XSPI PSRAM Polling Transfer with Clang"
echo "============================================================"

# Configuration
PROJECT_NAME="xspi_psram_polling_transfer_cm33_core0"
BUILD_DIR="build_clang_simple"
SDK_PATH="../__repo__"

# Check if Clang is available
if ! command -v clang &> /dev/null; then
    echo "❌ Clang not found. Please install Clang."
    exit 1
fi

echo "✅ Using Clang: $(clang --version | head -1)"

# Check if ARM GCC is available for libraries
if ! command -v arm-none-eabi-gcc &> /dev/null; then
    echo "❌ ARM GCC not found. Please install gcc-arm-none-eabi."
    exit 1
fi

echo "✅ Using ARM GCC: $(arm-none-eabi-gcc --version | head -1)"

# Create build directory
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

echo "📁 Build directory: $(pwd)"

# Compiler settings
CC="clang"
OBJCOPY="llvm-objcopy"
if ! command -v llvm-objcopy &> /dev/null; then
    OBJCOPY="arm-none-eabi-objcopy"
fi

# Target and architecture flags
TARGET_FLAGS="-target arm-none-eabi"
ARCH_FLAGS="-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16"
SYSROOT_FLAGS="--sysroot=/usr/lib/arm-none-eabi"

# Include directories
INCLUDES=(
    "-I../."
    "-I${SDK_PATH}/devices/MIMXRT798S"
    "-I${SDK_PATH}/devices/MIMXRT798S/drivers"
    "-I${SDK_PATH}/devices/MIMXRT798S/utilities"
    "-I${SDK_PATH}/devices/MIMXRT798S/utilities/debug_console_lite"
    "-I${SDK_PATH}/devices/MIMXRT798S/utilities/str"
    "-I${SDK_PATH}/CMSIS/Core/Include"
    "-I${SDK_PATH}/CMSIS/DSP/Include"
    "-I${SDK_PATH}/components/uart"
    "-I${SDK_PATH}/boards/mimxrt700evk"
    "-I${SDK_PATH}/boards/mimxrt700evk/flash_config"
)

# Preprocessor definitions
DEFINES=(
    "-DCPU_MIMXRT798SGFOA_cm33_core0"
    "-DCPU_MIMXRT798SGFOA"
    "-DARM_MATH_CM33"
    "-D__FPU_PRESENT=1U"
    "-D__DSP_PRESENT=1U"
    "-DSERIAL_PORT_TYPE_UART=1"
    "-DSDK_DEBUGCONSOLE=1"
    "-DCR_INTEGER_PRINTF"
    "-DPRINTF_FLOAT_ENABLE=0"
    "-D__MCUXPRESSO"
    "-D__USE_CMSIS"
    "-DDEBUG"
    "-D__STARTUP_CLEAR_BSS"
    "-D__STARTUP_INITIALIZE_NONCACHEDATA"
    "-D__STARTUP_INITIALIZE_RAMFUNCTION"
)

# Compiler flags
CFLAGS=(
    ${TARGET_FLAGS}
    ${ARCH_FLAGS}
    ${SYSROOT_FLAGS}
    "-g3"
    "-O0"
    "-fno-common"
    "-ffunction-sections"
    "-fdata-sections"
    "-fno-builtin"
    "-fmerge-all-constants"
    "-mapcs"
    "-std=gnu99"
    "-Wall"
    "-Wextra"
    "-Wno-unused-parameter"
    "-Wno-unused-function"
)

# Linker flags
LDFLAGS=(
    ${TARGET_FLAGS}
    ${ARCH_FLAGS}
    ${SYSROOT_FLAGS}
    "-fuse-ld=lld"
    "-Wl,--gc-sections"
    "-Wl,--print-memory-usage"
    "-Wl,-Map=${PROJECT_NAME}.map"
    "-T../MIMXRT798Sxxxx_cm33_core0_flash.ld"
    "-L/usr/lib/arm-none-eabi/lib/thumb/v8-m.main+fp/hard"
    "-L/usr/lib/gcc/arm-none-eabi/13.2.1/thumb/v8-m.main+fp/hard"
)

# Source files
SOURCES=(
    "../xspi_psram_polling_transfer.c"
    "../xspi_psram_ops.c"
    "../board.c"
    "../clock_config.c"
    "../pin_mux.c"
    "../hardware_init.c"
    "${SDK_PATH}/devices/MIMXRT798S/system_MIMXRT798S_cm33_core0.c"
    "${SDK_PATH}/devices/MIMXRT798S/gcc/startup_MIMXRT798S_cm33_core0.S"
    "${SDK_PATH}/devices/MIMXRT798S/drivers/fsl_common.c"
    "${SDK_PATH}/devices/MIMXRT798S/drivers/fsl_common_arm.c"
    "${SDK_PATH}/devices/MIMXRT798S/drivers/fsl_clock.c"
    "${SDK_PATH}/devices/MIMXRT798S/drivers/fsl_power.c"
    "${SDK_PATH}/devices/MIMXRT798S/drivers/fsl_reset.c"
    "${SDK_PATH}/devices/MIMXRT798S/drivers/fsl_gpio.c"
    "${SDK_PATH}/devices/MIMXRT798S/drivers/fsl_lpflexcomm.c"
    "${SDK_PATH}/devices/MIMXRT798S/drivers/fsl_lpuart.c"
    "${SDK_PATH}/devices/MIMXRT798S/drivers/fsl_xspi.c"
    "${SDK_PATH}/devices/MIMXRT798S/drivers/fsl_cache.c"
    "${SDK_PATH}/devices/MIMXRT798S/drivers/fsl_glikey.c"
    "${SDK_PATH}/devices/MIMXRT798S/utilities/debug_console_lite/fsl_debug_console.c"
    "${SDK_PATH}/devices/MIMXRT798S/utilities/debug_console_lite/fsl_assert.c"
    "${SDK_PATH}/devices/MIMXRT798S/utilities/str/fsl_str.c"
    "${SDK_PATH}/devices/MIMXRT798S/utilities/fsl_sbrk.c"
    "${SDK_PATH}/devices/MIMXRT798S/utilities/fsl_syscall_stub.c"
    "${SDK_PATH}/devices/MIMXRT798S/utilities/fsl_memcpy.S"
    "${SDK_PATH}/components/uart/fsl_adapter_lpuart.c"
    "${SDK_PATH}/boards/mimxrt700evk/flash_config/flash_config.c"
)

# Libraries
LIBS=("-lm" "-lc" "-lgcc" "-lnosys")

echo "🔧 Compiling source files..."

# Compile each source file
OBJECTS=()
for src in "${SOURCES[@]}"; do
    if [[ ! -f "$src" ]]; then
        echo "⚠️  Source file not found: $src"
        continue
    fi
    
    # Get object file name
    obj_name=$(basename "$src")
    obj_name="${obj_name%.*}.o"
    
    echo "   Compiling: $(basename "$src")"
    
    # Compile
    ${CC} "${CFLAGS[@]}" "${INCLUDES[@]}" "${DEFINES[@]}" -c "$src" -o "$obj_name"
    
    OBJECTS+=("$obj_name")
done

echo "✅ Compiled ${#OBJECTS[@]} object files"

echo "🔗 Linking executable..."

# Link
${CC} "${LDFLAGS[@]}" "${OBJECTS[@]}" "${LIBS[@]}" -o "${PROJECT_NAME}.elf"

echo "✅ Linked: ${PROJECT_NAME}.elf"

# Generate binary
echo "📦 Generating binary..."
${OBJCOPY} -O binary "${PROJECT_NAME}.elf" "${PROJECT_NAME}.bin"

echo "✅ Generated: ${PROJECT_NAME}.bin"

# Show file sizes
echo "📊 Build Results:"
echo "   ELF size: $(ls -lh ${PROJECT_NAME}.elf | awk '{print $5}')"
echo "   BIN size: $(ls -lh ${PROJECT_NAME}.bin | awk '{print $5}')"

# Show memory usage if map file exists
if [[ -f "${PROJECT_NAME}.map" ]]; then
    echo "   Map file: ${PROJECT_NAME}.map"
fi

echo "🎉 Build completed successfully!"
echo "   Output: $(pwd)/${PROJECT_NAME}.elf"
