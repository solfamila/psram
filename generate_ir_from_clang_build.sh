#!/bin/bash

# Generate LLVM IR using the EXACT same flags from the successful Clang build
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
LLVM_INSTALL_DIR="$PROJECT_ROOT/llvm-install"
PERIPHERAL_ANALYZER="$PROJECT_ROOT/llvm_analysis_pass/build/bin/peripheral-analyzer"
IR_OUTPUT_DIR="$PROJECT_ROOT/clang_ir_final"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

echo "=============================================================="
echo "Generate LLVM IR from MIMXRT700 Clang Build"
echo "=============================================================="

mkdir -p "$IR_OUTPUT_DIR"

# Exact compilation flags from the successful Clang build
CLANG_BIN="$LLVM_INSTALL_DIR/bin/clang"
C_FLAGS="-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 --sysroot=/usr/lib/arm-none-eabi -I/usr/lib/arm-none-eabi/include -mfloat-abi=hard -mfpu=fpv5-sp-d16 --sysroot=/opt/gcc-arm-none-eabi-10.3-2021.10/arm-none-eabi -I/opt/gcc-arm-none-eabi-10.3-2021.10/arm-none-eabi/include -DDEBUG -DCPU_MIMXRT798SGFOA_cm33_core0 -DARM_MATH_CM33 -D__FPU_PRESENT=1 -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -D__REDLIB__ -DMCUXPRESSO_SDK -g -O0 -mcpu=cortex-m33 -mthumb -Wall -fno-common -ffunction-sections -fdata-sections -fno-builtin -std=gnu99 -target arm-none-eabi"

C_INCLUDES="-I$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0 -I$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/CMSIS/Core/Include/. -I$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/CMSIS/Core/Include/m-profile -I$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/components/uart/. -I$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/. -I$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/periph -I$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/../periph -I$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/drivers/. -I$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/boards/mimxrt700evk/flash_config/. -I$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/utilities/debug_console_lite/. -I$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/utilities/debug_console_lite -I$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/utilities/str"

# Project source files to analyze
PROJECT_SOURCES=(
    "$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/xspi_psram_polling_transfer.c"
    "$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/xspi_psram_ops.c"
    "$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/board.c"
    "$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/clock_config.c"
    "$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/pin_mux.c"
    "$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/hardware_init.c"
)

# Key SDK driver files
SDK_DRIVERS=(
    "$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/drivers/fsl_xspi.c"
    "$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/drivers/fsl_clock.c"
    "$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/drivers/fsl_power.c"
    "$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/drivers/fsl_reset.c"
    "$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/drivers/fsl_gpio.c"
    "$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/drivers/fsl_common.c"
)

# Function to generate LLVM IR for a source file
generate_ir() {
    local source_file="$1"
    local base_name=$(basename "$source_file" .c)
    local ir_file="$IR_OUTPUT_DIR/${base_name}.ll"
    local log_file="$IR_OUTPUT_DIR/${base_name}_compile.log"
    
    log_info "Generating LLVM IR for: $base_name.c"
    
    # Generate LLVM IR using the exact same flags, just add -S -emit-llvm
    if "$CLANG_BIN" \
        $C_FLAGS \
        $C_INCLUDES \
        -S -emit-llvm \
        -o "$ir_file" \
        "$source_file" \
        > "$log_file" 2>&1; then
        
        log_success "Generated: $base_name.ll"
        return 0
    else
        log_error "Failed to generate IR for: $base_name.c"
        log_info "Check log: $log_file"
        return 1
    fi
}

# Function to apply peripheral analysis
analyze_ir() {
    local ir_file="$1"
    local base_name=$(basename "$ir_file" .ll)
    local analysis_file="$IR_OUTPUT_DIR/${base_name}_analysis.json"
    
    log_info "Analyzing: $base_name.ll"
    
    if "$PERIPHERAL_ANALYZER" "$ir_file" > "$analysis_file" 2>&1; then
        # Check if analysis found anything
        local access_count=$(python3 -c "
import json
try:
    with open('$analysis_file', 'r') as f:
        data = json.load(f)
    print(len(data.get('peripheral_accesses', [])))
except:
    print(0)
")
        if [[ $access_count -gt 0 ]]; then
            log_success "Found $access_count peripheral accesses in $base_name.ll"
        else
            log_info "No peripheral accesses found in $base_name.ll"
        fi
        return 0
    else
        log_warning "Analysis failed for: $base_name.ll"
        return 1
    fi
}

# Main execution
main() {
    log_info "Starting LLVM IR generation and analysis..."
    
    local ir_files=()
    local total_generated=0
    
    # Generate IR for project source files
    log_info "Generating IR for project source files..."
    for source_file in "${PROJECT_SOURCES[@]}"; do
        if [[ -f "$source_file" ]]; then
            if generate_ir "$source_file"; then
                local base_name=$(basename "$source_file" .c)
                ir_files+=("$IR_OUTPUT_DIR/${base_name}.ll")
                ((total_generated++))
            fi
        else
            log_warning "Source file not found: $source_file"
        fi
    done
    
    # Generate IR for SDK driver files
    log_info "Generating IR for SDK driver files..."
    for source_file in "${SDK_DRIVERS[@]}"; do
        if [[ -f "$source_file" ]]; then
            if generate_ir "$source_file"; then
                local base_name=$(basename "$source_file" .c)
                ir_files+=("$IR_OUTPUT_DIR/${base_name}.ll")
                ((total_generated++))
            fi
        else
            log_warning "SDK driver file not found: $source_file"
        fi
    done
    
    log_success "Generated $total_generated LLVM IR files"
    
    # Apply peripheral analysis to all IR files
    log_info "Applying peripheral analysis pass..."
    local total_analyzed=0
    local total_accesses=0
    
    for ir_file in "${ir_files[@]}"; do
        if [[ -f "$ir_file" ]]; then
            if analyze_ir "$ir_file"; then
                ((total_analyzed++))
                
                # Count accesses
                local base_name=$(basename "$ir_file" .ll)
                local analysis_file="$IR_OUTPUT_DIR/${base_name}_analysis.json"
                local access_count=$(python3 -c "
import json
try:
    with open('$analysis_file', 'r') as f:
        data = json.load(f)
    print(len(data.get('peripheral_accesses', [])))
except:
    print(0)
")
                ((total_accesses += access_count))
            fi
        fi
    done
    
    # Generate combined analysis report
    log_info "Generating combined analysis report..."
    
    python3 << EOF
import json
import glob
import os

combined_data = {
    "toolchain": "clang",
    "total_files_analyzed": $total_analyzed,
    "total_accesses": 0,
    "peripheral_accesses": [],
    "summary": {"peripherals": {}}
}

# Find all analysis files
analysis_files = glob.glob("$IR_OUTPUT_DIR/*_analysis.json")

for file_path in analysis_files:
    try:
        with open(file_path, 'r') as f:
            data = json.load(f)
            
        if 'peripheral_accesses' in data:
            for access in data['peripheral_accesses']:
                access['source_file'] = os.path.basename(file_path).replace('_analysis.json', '')
                combined_data['peripheral_accesses'].append(access)
                
    except Exception as e:
        print(f"Error processing {file_path}: {e}")

# Update summary
combined_data['total_accesses'] = len(combined_data['peripheral_accesses'])

# Count peripherals
peripheral_counts = {}
for access in combined_data['peripheral_accesses']:
    peripheral = access.get('peripheral', 'Unknown')
    peripheral_counts[peripheral] = peripheral_counts.get(peripheral, 0) + 1

combined_data['summary']['peripherals'] = peripheral_counts

with open("$IR_OUTPUT_DIR/combined_analysis.json", 'w') as f:
    json.dump(combined_data, f, indent=2)

print(f"\\nFINAL RESULTS:")
print(f"Files analyzed: {combined_data['total_files_analyzed']}")
print(f"Total peripheral accesses: {combined_data['total_accesses']}")
print(f"Unique peripherals: {len(peripheral_counts)}")

if peripheral_counts:
    print("\\nPeripheral breakdown:")
    for peripheral, count in sorted(peripheral_counts.items(), key=lambda x: x[1], reverse=True):
        print(f"  {peripheral}: {count} accesses")

# Show sample accesses
accesses = combined_data['peripheral_accesses']
if accesses:
    print("\\nSample peripheral accesses:")
    for access in accesses[:10]:
        peripheral = access.get('peripheral', 'Unknown')
        register = access.get('register', 'Unknown')
        function = access.get('function', 'Unknown')
        source = access.get('source_file', 'Unknown')
        print(f"  {peripheral}.{register} in {function} ({source})")
    
    if len(accesses) > 10:
        print(f"  ... and {len(accesses) - 10} more accesses")
EOF
    
    log_success "Analysis completed!"
    log_info "Results saved in: $IR_OUTPUT_DIR"
    log_info "Combined analysis: $IR_OUTPUT_DIR/combined_analysis.json"
}

# Execute main function
main "$@"
