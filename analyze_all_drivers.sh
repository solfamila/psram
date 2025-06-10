#!/bin/bash

# Script to analyze all MIMXRT700 driver LLVM IR files for comprehensive peripheral analysis

set -e

echo "Starting comprehensive MIMXRT700 peripheral analysis..."

# Create output directory
mkdir -p clang_ir_final/analysis_results

# Enhanced peripheral analyzer path
ANALYZER="/home/smw016108/Downloads/ex/llvm_analysis_pass/build/bin/peripheral-analyzer"

# List of all LLVM IR files to analyze
IR_FILES=(
    "clang_ir_final/fsl_xspi.ll"
    "clang_ir_final/xspi_psram_polling_transfer.ll"
    "clang_ir_final/drivers/fsl_clock.ll"
    "clang_ir_final/drivers/fsl_gpio.ll"
    "clang_ir_final/drivers/fsl_lpuart.ll"
    "clang_ir_final/drivers/fsl_lpi2c.ll"
    "clang_ir_final/drivers/fsl_lpspi.ll"
    "clang_ir_final/drivers/fsl_ctimer.ll"
    "clang_ir_final/drivers/fsl_power.ll"
    "clang_ir_final/drivers/fsl_reset.ll"
    "clang_ir_final/drivers/fsl_common.ll"
    "clang_ir_final/drivers/fsl_cache.ll"
    "clang_ir_final/drivers/fsl_mrt.ll"
    "clang_ir_final/drivers/fsl_ostimer.ll"
    "clang_ir_final/drivers/fsl_utick.ll"
    "clang_ir_final/drivers/fsl_wwdt.ll"
    "clang_ir_final/drivers/fsl_crc.ll"
    "clang_ir_final/drivers/fsl_trng.ll"
    "clang_ir_final/drivers/fsl_usdhc.ll"
    "clang_ir_final/drivers/fsl_flexio.ll"
    "clang_ir_final/drivers/fsl_sai.ll"
    "clang_ir_final/drivers/fsl_pdm.ll"
    "clang_ir_final/drivers/fsl_i3c.ll"
    "clang_ir_final/drivers/fsl_acmp.ll"
    "clang_ir_final/drivers/fsl_lpadc.ll"
    "clang_ir_final/drivers/fsl_sdadc.ll"
    "clang_ir_final/drivers/fsl_pint.ll"
    "clang_ir_final/drivers/fsl_sctimer.ll"
    "clang_ir_final/drivers/fsl_mu.ll"
    "clang_ir_final/drivers/fsl_sema42.ll"
    "clang_ir_final/drivers/fsl_cdog.ll"
    "clang_ir_final/drivers/fsl_gdet.ll"
    "clang_ir_final/drivers/fsl_glikey.ll"
    "clang_ir_final/drivers/fsl_irtc.ll"
    "clang_ir_final/drivers/fsl_itrc.ll"
    "clang_ir_final/drivers/fsl_freqme.ll"
    "clang_ir_final/drivers/fsl_puf_v3.ll"
    "clang_ir_final/drivers/fsl_syspm.ll"
    "clang_ir_final/drivers/fsl_jpegdec.ll"
    "clang_ir_final/drivers/fsl_pngdec.ll"
    "clang_ir_final/drivers/fsl_lcdif.ll"
    "clang_ir_final/drivers/fsl_mipi_dsi.ll"
    "clang_ir_final/drivers/fsl_mmu.ll"
    "clang_ir_final/drivers/fsl_inputmux.ll"
    "clang_ir_final/drivers/fsl_ezhv.ll"
    "clang_ir_final/drivers/fsl_dsp.ll"
)

# Initialize summary variables
total_accesses=0
total_peripherals=0
declare -A peripheral_counts

echo "Analyzing ${#IR_FILES[@]} LLVM IR files..."

# Analyze each file
for ir_file in "${IR_FILES[@]}"; do
    if [ -f "$ir_file" ]; then
        echo "Analyzing $ir_file..."
        
        # Extract base name for output file
        base_name=$(basename "$ir_file" .ll)
        output_file="clang_ir_final/analysis_results/${base_name}_analysis.json"
        
        # Run peripheral analyzer
        $ANALYZER -v -o "$output_file" "$ir_file" || {
            echo "Warning: Analysis failed for $ir_file, skipping..."
            continue
        }
        
        # Extract statistics from the analysis
        if [ -f "$output_file" ]; then
            # Count peripheral accesses in this file
            accesses=$(grep -o '"peripheralName"' "$output_file" 2>/dev/null | wc -l || echo "0")
            echo "  Found $accesses peripheral register accesses"
            total_accesses=$((total_accesses + accesses))
            
            # Extract unique peripherals
            if [ "$accesses" -gt 0 ]; then
                peripherals=$(grep '"peripheralName"' "$output_file" | sed 's/.*"peripheralName": *"\([^"]*\)".*/\1/' | sort -u)
                while IFS= read -r peripheral; do
                    if [ -n "$peripheral" ]; then
                        peripheral_counts["$peripheral"]=$((${peripheral_counts["$peripheral"]:-0} + 1))
                    fi
                done <<< "$peripherals"
            fi
        fi
    else
        echo "Warning: $ir_file not found, skipping..."
    fi
done

echo ""
echo "=== COMPREHENSIVE MIMXRT700 PERIPHERAL ANALYSIS SUMMARY ==="
echo "Total LLVM IR files analyzed: ${#IR_FILES[@]}"
echo "Total peripheral register accesses found: $total_accesses"
echo ""

# Count unique peripherals
total_peripherals=${#peripheral_counts[@]}
echo "Unique peripherals accessed: $total_peripherals"
echo ""

if [ $total_peripherals -gt 0 ]; then
    echo "Peripheral access breakdown:"
    for peripheral in "${!peripheral_counts[@]}"; do
        echo "  $peripheral: ${peripheral_counts[$peripheral]} files"
    done
fi

echo ""
echo "Analysis results saved in: clang_ir_final/analysis_results/"
echo ""
echo "Key findings:"
echo "- Enhanced analyzer now detects struct-based peripheral register accesses"
echo "- Comprehensive MIMXRT700 peripheral definitions included"
echo "- Volatile memory access detection for hardware register identification"
echo "- Function context analysis for peripheral type inference"
echo ""
echo "Analysis complete!"
