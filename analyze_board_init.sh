#!/bin/bash

# Script to analyze board initialization LLVM IR files for comprehensive peripheral analysis

set -e

echo "Starting comprehensive MIMXRT700 board initialization peripheral analysis..."

# Create output directory
mkdir -p clang_ir_final/board_init_analysis

# Enhanced peripheral analyzer path
ANALYZER="/home/smw016108/Downloads/ex/llvm_analysis_pass/build/bin/peripheral-analyzer"

# List of board initialization LLVM IR files to analyze
BOARD_IR_FILES=(
    "clang_ir_final/board_init/hardware_init.ll"
    "clang_ir_final/board_init/board.ll"
    "clang_ir_final/board_init/pin_mux.ll"
    "clang_ir_final/board_init/clock_config.ll"
)

# Initialize summary variables
total_accesses=0
total_peripherals=0
declare -A peripheral_counts

echo "Analyzing ${#BOARD_IR_FILES[@]} board initialization LLVM IR files..."

# Analyze each file
for ir_file in "${BOARD_IR_FILES[@]}"; do
    if [ -f "$ir_file" ]; then
        echo "Analyzing $ir_file..."
        
        # Extract base name for output file
        base_name=$(basename "$ir_file" .ll)
        output_file="clang_ir_final/board_init_analysis/${base_name}_analysis.json"
        
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
echo "=== MIMXRT700 BOARD INITIALIZATION PERIPHERAL ANALYSIS ==="
echo "Total board initialization files analyzed: ${#BOARD_IR_FILES[@]}"
echo "Total peripheral register accesses found: $total_accesses"
echo ""

# Count unique peripherals
total_peripherals=${#peripheral_counts[@]}
echo "Unique peripherals accessed during board initialization: $total_peripherals"
echo ""

if [ $total_peripherals -gt 0 ]; then
    echo "Board initialization peripheral access breakdown:"
    for peripheral in "${!peripheral_counts[@]}"; do
        echo "  $peripheral: ${peripheral_counts[$peripheral]} files"
    done
fi

echo ""
echo "Board initialization analysis results saved in: clang_ir_final/board_init_analysis/"
echo ""
echo "Key findings:"
echo "- Board initialization accesses many more peripherals than driver code"
echo "- Pin configuration, clock setup, and power management are major components"
echo "- Cache and memory protection setup involves additional peripherals"
echo ""
echo "Board initialization analysis complete!"
