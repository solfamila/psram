#!/bin/bash

# Script to compile all MIMXRT700 driver files to LLVM IR for comprehensive peripheral analysis

set -e

# Create output directory
mkdir -p clang_ir_final/drivers

# Common Clang flags for MIMXRT700
CLANG_FLAGS="-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 \
--sysroot=/usr/lib/arm-none-eabi -I/usr/lib/arm-none-eabi/include \
-mfloat-abi=hard -mfpu=fpv5-sp-d16 \
--sysroot=/opt/gcc-arm-none-eabi-10.3-2021.10/arm-none-eabi \
-I/opt/gcc-arm-none-eabi-10.3-2021.10/arm-none-eabi/include \
-DDEBUG -DCPU_MIMXRT798SGFOA_cm33_core0 -DARM_MATH_CM33 -D__FPU_PRESENT=1 \
-DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF \
-DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -D__REDLIB__ -DMCUXPRESSO_SDK \
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
-I/home/smw016108/Downloads/ex/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/utilities/str"

CLANG_PATH="/home/smw016108/Downloads/ex/llvm-install/bin/clang"
DRIVERS_DIR="mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/drivers"

echo "Compiling MIMXRT700 drivers to LLVM IR..."

# List of key driver files to compile
DRIVER_FILES=(
    "fsl_clock.c"
    "fsl_gpio.c"
    "fsl_lpuart.c"
    "fsl_lpi2c.c"
    "fsl_lpspi.c"
    "fsl_ctimer.c"
    "fsl_dma.c"
    "fsl_power.c"
    "fsl_reset.c"
    "fsl_common.c"
    "fsl_cache.c"
    "fsl_mrt.c"
    "fsl_ostimer.c"
    "fsl_utick.c"
    "fsl_wwdt.c"
    "fsl_crc.c"
    "fsl_trng.c"
    "fsl_usdhc.c"
    "fsl_flexio.c"
    "fsl_sai.c"
    "fsl_pdm.c"
    "fsl_i3c.c"
    "fsl_acmp.c"
    "fsl_lpadc.c"
    "fsl_sdadc.c"
    "fsl_pint.c"
    "fsl_sctimer.c"
    "fsl_mu.c"
    "fsl_sema42.c"
    "fsl_cdog.c"
    "fsl_gdet.c"
    "fsl_glikey.c"
    "fsl_irtc.c"
    "fsl_itrc.c"
    "fsl_freqme.c"
    "fsl_puf_v3.c"
    "fsl_syspm.c"
    "fsl_jpegdec.c"
    "fsl_pngdec.c"
    "fsl_lcdif.c"
    "fsl_mipi_dsi.c"
    "fsl_mmu.c"
    "fsl_inputmux.c"
    "fsl_ezhv.c"
    "fsl_dsp.c"
)

# Compile each driver file
for driver in "${DRIVER_FILES[@]}"; do
    if [ -f "$DRIVERS_DIR/$driver" ]; then
        echo "Compiling $driver..."
        output_name=$(basename "$driver" .c)
        $CLANG_PATH $CLANG_FLAGS -S -emit-llvm -o "clang_ir_final/drivers/${output_name}.ll" "$DRIVERS_DIR/$driver" || {
            echo "Warning: Failed to compile $driver, skipping..."
            continue
        }
        echo "Generated: clang_ir_final/drivers/${output_name}.ll"
    else
        echo "Warning: $driver not found, skipping..."
    fi
done

echo "Driver compilation complete!"
echo "Generated LLVM IR files in clang_ir_final/drivers/"
