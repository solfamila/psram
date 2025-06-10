#!/bin/bash

# Enhanced LLVM Peripheral Analysis - Chronological Execution Order
# This script analyzes peripheral register accesses in their actual execution order
# Critical for embedded systems programming where initialization sequence matters

set -e

echo "=== MIMXRT700 CHRONOLOGICAL PERIPHERAL ANALYSIS ==="
echo "Analyzing peripheral register accesses in execution order..."
echo ""

# Build the enhanced analysis pass if needed
if [ ! -f "llvm_analysis_pass/build/lib/libPeripheralAnalysisPass.so" ]; then
    echo "Building enhanced LLVM peripheral analysis pass..."
    cd llvm_analysis_pass
    mkdir -p build && cd build
    cmake ..
    make -j$(nproc)
    cd ../..
    echo "✓ Enhanced analysis pass built successfully"
    echo ""
fi

# Create output directory for chronological analysis
mkdir -p chronological_analysis_results

echo "=== PHASE 1: BOARD INITIALIZATION ANALYSIS ==="
echo "Analyzing board initialization peripheral accesses..."

# Analyze board initialization files in execution order
board_init_files=(
    "clang_ir_final/board_init/hardware_init.ll"
    "clang_ir_final/board_init/clock_config.ll" 
    "clang_ir_final/board_init/pin_mux.ll"
    "clang_ir_final/board_init/board.ll"
)

board_init_count=0
for file in "${board_init_files[@]}"; do
    if [ -f "$file" ]; then
        echo "  Analyzing $(basename $file)..."
        output_file="chronological_analysis_results/$(basename $file .ll)_chronological.json"
        
        # Run enhanced analysis with chronological export
        llvm_analysis_pass/build/bin/peripheral-analyzer \
            -v \
            -o "$output_file" \
            --chronological \
            "$file" 2>/dev/null || echo "    Warning: Analysis failed for $file"
        
        if [ -f "$output_file" ]; then
            accesses=$(python3 -c "
import json
try:
    with open('$output_file', 'r') as f:
        data = json.load(f)
    if 'chronological_sequence' in data:
        print(len(data['chronological_sequence']))
    else:
        print(0)
except:
    print(0)
" 2>/dev/null)
            echo "    ✓ Found $accesses peripheral accesses"
            board_init_count=$((board_init_count + accesses))
        fi
    fi
done

echo "✓ Board initialization analysis complete: $board_init_count total accesses"
echo ""

echo "=== PHASE 2: DRIVER INITIALIZATION ANALYSIS ==="
echo "Analyzing driver initialization peripheral accesses..."

# Analyze key driver files that contain initialization code
driver_init_files=(
    "clang_ir_final/drivers/fsl_xspi.ll"
    "clang_ir_final/drivers/fsl_clock.ll"
    "clang_ir_final/drivers/fsl_gpio.ll"
    "clang_ir_final/drivers/fsl_power.ll"
    "clang_ir_final/drivers/fsl_reset.ll"
)

driver_init_count=0
for file in "${driver_init_files[@]}"; do
    if [ -f "$file" ]; then
        echo "  Analyzing $(basename $file)..."
        output_file="chronological_analysis_results/$(basename $file .ll)_chronological.json"
        
        llvm_analysis_pass/build/bin/peripheral-analyzer \
            -v \
            -o "$output_file" \
            --chronological \
            "$file" 2>/dev/null || echo "    Warning: Analysis failed for $file"
        
        if [ -f "$output_file" ]; then
            accesses=$(python3 -c "
import json
try:
    with open('$output_file', 'r') as f:
        data = json.load(f)
    if 'chronological_sequence' in data:
        print(len(data['chronological_sequence']))
    else:
        print(0)
except:
    print(0)
" 2>/dev/null)
            echo "    ✓ Found $accesses peripheral accesses"
            driver_init_count=$((driver_init_count + accesses))
        fi
    fi
done

echo "✓ Driver initialization analysis complete: $driver_init_count total accesses"
echo ""

echo "=== PHASE 3: RUNTIME OPERATION ANALYSIS ==="
echo "Analyzing runtime peripheral accesses..."

# Analyze runtime operation files
runtime_files=(
    "clang_ir_final/drivers/xspi_psram_polling_transfer.ll"
)

runtime_count=0
for file in "${runtime_files[@]}"; do
    if [ -f "$file" ]; then
        echo "  Analyzing $(basename $file)..."
        output_file="chronological_analysis_results/$(basename $file .ll)_chronological.json"
        
        llvm_analysis_pass/build/bin/peripheral-analyzer \
            -v \
            -o "$output_file" \
            --chronological \
            "$file" 2>/dev/null || echo "    Warning: Analysis failed for $file"
        
        if [ -f "$output_file" ]; then
            accesses=$(python3 -c "
import json
try:
    with open('$output_file', 'r') as f:
        data = json.load(f)
    if 'chronological_sequence' in data:
        print(len(data['chronological_sequence']))
    else:
        print(0)
except:
    print(0)
" 2>/dev/null)
            echo "    ✓ Found $accesses peripheral accesses"
            runtime_count=$((runtime_count + accesses))
        fi
    fi
done

echo "✓ Runtime operation analysis complete: $runtime_count total accesses"
echo ""

echo "=== PHASE 4: COMPREHENSIVE CHRONOLOGICAL SEQUENCE GENERATION ==="
echo "Generating complete chronological execution sequence..."

# Combine all chronological results into a master sequence
python3 << 'EOF'
import json
import os
import glob

def load_chronological_data(file_path):
    try:
        with open(file_path, 'r') as f:
            data = json.load(f)
        return data.get('chronological_sequence', [])
    except:
        return []

# Collect all chronological sequences
all_sequences = []
result_files = glob.glob('chronological_analysis_results/*_chronological.json')

for file_path in result_files:
    sequences = load_chronological_data(file_path)
    all_sequences.extend(sequences)

# Sort by sequence number to maintain global chronological order
all_sequences.sort(key=lambda x: x.get('sequence_number', 0))

# Group by execution phase for analysis
phase_groups = {
    'board_init': [],
    'driver_init': [],
    'runtime': []
}

for seq in all_sequences:
    phase = seq.get('execution_phase', 'runtime')
    if phase in phase_groups:
        phase_groups[phase].append(seq)

# Generate comprehensive chronological report
master_report = {
    'analysis_type': 'comprehensive_chronological_peripheral_sequence',
    'total_accesses': len(all_sequences),
    'description': 'Complete peripheral register access sequence in chronological execution order',
    'execution_phases': {
        'board_initialization': len(phase_groups['board_init']),
        'driver_initialization': len(phase_groups['driver_init']),
        'runtime_operation': len(phase_groups['runtime'])
    },
    'chronological_sequence': all_sequences
}

# Save master chronological sequence
with open('chronological_analysis_results/master_chronological_sequence.json', 'w') as f:
    json.dump(master_report, f, indent=2)

print(f"✓ Master chronological sequence generated: {len(all_sequences)} total accesses")
print(f"  - Board initialization: {len(phase_groups['board_init'])} accesses")
print(f"  - Driver initialization: {len(phase_groups['driver_init'])} accesses") 
print(f"  - Runtime operation: {len(phase_groups['runtime'])} accesses")
EOF

echo ""

echo "=== CHRONOLOGICAL ANALYSIS COMPLETE ==="
total_accesses=$((board_init_count + driver_init_count + runtime_count))
echo "Total peripheral register accesses analyzed: $total_accesses"
echo ""
echo "Results saved in chronological_analysis_results/:"
echo "  - Individual phase analysis files (*_chronological.json)"
echo "  - master_chronological_sequence.json (complete sequence)"
echo ""
echo "To view the chronological sequence:"
echo "  cat chronological_analysis_results/master_chronological_sequence.json | head -100"
echo ""
echo "To see execution order summary:"
echo "  python3 -c \"
import json
with open('chronological_analysis_results/master_chronological_sequence.json', 'r') as f:
    data = json.load(f)
print('=== CHRONOLOGICAL EXECUTION SEQUENCE ===')
for i, access in enumerate(data['chronological_sequence'][:20]):
    print(f'{i+1:3d}. {access[\"execution_phase\"]:12s} | {access[\"peripheral_name\"]:8s} | {access[\"register_name\"]:15s} | {access[\"access_type\"]:12s} | {access[\"source_location\"][\"function\"]}()')
if len(data['chronological_sequence']) > 20:
    print(f'... and {len(data[\"chronological_sequence\"]) - 20} more accesses')
\""

echo ""
echo "🎯 CHRONOLOGICAL PERIPHERAL ANALYSIS COMPLETE!"
echo "   This shows the exact execution order: board init → driver init → runtime"
