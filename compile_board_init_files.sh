#!/bin/bash

# Script to compile board initialization files to LLVM IR for comprehensive peripheral analysis

set -e

echo "Compiling MIMXRT700 board initialization files to LLVM IR..."

# Create output directory
mkdir -p clang_ir_final/board_init

# Common Clang flags for MIMXRT700
CLANG_FLAGS="-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 \
--sysroot=/usr/lib/arm-none-eabi -I/usr/lib/arm-none-eabi/include \
-mfloat-abi=hard -mfpu=fpv5-sp-d16 \
--sysroot=/opt/gcc-arm-none-eabi-10.3-2021.10/arm-none-eabi \
-I/opt/gcc-arm-none-eabi-10.3-2021.10/arm-none-eabi/include \
-DDEBUG -DCPU_MIMXRT798SGFOA_cm33_core0 -DARM_MATH_CM33 -D__FPU_PRESENT=1 \
-DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF \
-DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -D__REDLIB__ -DMCUXPRESSO_SDK \
-DDEMO_USE_XSPI2=1 \
-g -O0 -mcpu=cortex-m33 -mthumb -Wall -fno-common -ffunction-sections \
-fdata-sections -fno-builtin -std=gnu99 -target arm-none-eabi \
-I/home/smw016108/Downloads/ex/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0 \
-I/home/smw016108/Downloads/ex/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/CMSIS/Core/Include/. \
-I/home/smw016108/Downloads/ex/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/CMSIS/Core/Include/m-profile \
-I/home/smw016108/Downloads/ex/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/components/uart/. \
-I/home/smw016108/Downloads/ex/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/. \
-I/home/smw016108/Downloads/ex/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/periph \
-I/home/smw016108/Downloads/ex/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/../periph \
-I/home/smw016108/Downloads/ex/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/drivers/. \
-I/home/smw016108/Downloads/ex/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/boards/mimxrt700evk/flash_config/. \
-I/home/smw016108/Downloads/ex/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/utilities/debug_console_lite/. \
-I/home/smw016108/Downloads/ex/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/utilities/debug_console_lite \
-I/home/smw016108/Downloads/ex/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/utilities/str \
-I/home/smw016108/Downloads/ex/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/boards/mimxrt700evk/driver_examples/xspi/psram/polling_transfer/cm33_core0/."

CLANG_PATH="/home/smw016108/Downloads/ex/llvm-install/bin/clang"

# Board initialization files to compile
BOARD_INIT_FILES=(
    "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/hardware_init.c"
    "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/boards/mimxrt700evk/driver_examples/xspi/psram/polling_transfer/cm33_core0/board.c"
    "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/boards/mimxrt700evk/driver_examples/xspi/psram/polling_transfer/cm33_core0/pin_mux.c"
    "mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/boards/mimxrt700evk/driver_examples/xspi/psram/polling_transfer/cm33_core0/clock_config.c"
)

# Compile each board initialization file
for board_file in "${BOARD_INIT_FILES[@]}"; do
    if [ -f "$board_file" ]; then
        echo "Compiling $board_file..."
        output_name=$(basename "$board_file" .c)
        $CLANG_PATH $CLANG_FLAGS -S -emit-llvm -o "clang_ir_final/board_init/${output_name}.ll" "$board_file" || {
            echo "Warning: Failed to compile $board_file, skipping..."
            continue
        }
        echo "Generated: clang_ir_final/board_init/${output_name}.ll"
    else
        echo "Warning: $board_file not found, skipping..."
    fi
done

echo "Board initialization file compilation complete!"
echo "Generated LLVM IR files in clang_ir_final/board_init/"
