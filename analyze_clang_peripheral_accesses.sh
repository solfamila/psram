#!/bin/bash

# MIMXRT700 Clang Build Peripheral Analysis Script
# This script generates LLVM IR from Clang-compiled source files and applies peripheral analysis

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
CLANG_DEBUG_DIR="$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc/build_clang_debug"
CLANG_RELEASE_DIR="$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc/build_clang_release"
ANALYSIS_OUTPUT_DIR="$PROJECT_ROOT/clang_peripheral_analysis"
LLVM_INSTALL_DIR="$PROJECT_ROOT/llvm-install"
PERIPHERAL_ANALYZER="$PROJECT_ROOT/llvm_analysis_pass/build/bin/peripheral-analyzer"
SDK_ROOT="$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

echo "=============================================================="
echo "MIMXRT700 Clang Build Peripheral Analysis"
echo "=============================================================="
echo ""

# Create analysis output directory
mkdir -p "$ANALYSIS_OUTPUT_DIR"
log_info "Analysis results will be saved to: $ANALYSIS_OUTPUT_DIR"

# Verify prerequisites
if [[ ! -f "$PERIPHERAL_ANALYZER" ]]; then
    log_error "Peripheral analyzer not found at: $PERIPHERAL_ANALYZER"
    exit 1
fi

if [[ ! -f "$LLVM_INSTALL_DIR/bin/clang" ]]; then
    log_error "Clang not found at: $LLVM_INSTALL_DIR/bin/clang"
    exit 1
fi

# Define the main project source files to analyze
PROJECT_SOURCES=(
    "xspi_psram_polling_transfer.c"
    "xspi_psram_ops.c"
    "board.c"
    "clock_config.c"
    "pin_mux.c"
    "hardware_init.c"
)

# Define key SDK driver files to analyze
SDK_DRIVERS=(
    "fsl_xspi.c"
    "fsl_clock.c"
    "fsl_power.c"
    "fsl_reset.c"
    "fsl_gpio.c"
    "fsl_common.c"
)

# Function to extract object files from Clang build and analyze them
analyze_clang_object_files() {
    local build_dir="$1"
    local build_type="$2"

    log_info "Analyzing Clang object files from: $build_dir"

    # Find all object files in the Clang build directory
    local object_files=()
    while IFS= read -r -d '' file; do
        object_files+=("$file")
    done < <(find "$build_dir" -name "*.obj" -print0 2>/dev/null)

    if [[ ${#object_files[@]} -eq 0 ]]; then
        log_warning "No object files found in $build_dir"
        return 1
    fi

    log_info "Found ${#object_files[@]} object files to analyze"

    # Create a combined disassembly for analysis
    local combined_asm="$ANALYSIS_OUTPUT_DIR/clang_${build_type}_combined.s"
    echo "# Combined disassembly from Clang build" > "$combined_asm"
    echo "# Build type: $build_type" >> "$combined_asm"
    echo "# Generated on: $(date)" >> "$combined_asm"
    echo "" >> "$combined_asm"

    # Disassemble each object file and look for peripheral accesses
    local peripheral_accesses=()

    for obj_file in "${object_files[@]}"; do
        local base_name=$(basename "$obj_file" .obj)
        log_info "Analyzing object file: $base_name"

        # Use llvm-objdump to disassemble
        local asm_file="$ANALYSIS_OUTPUT_DIR/${base_name}_${build_type}.s"
        if "$LLVM_INSTALL_DIR/bin/llvm-objdump" -d "$obj_file" > "$asm_file" 2>/dev/null; then
            echo "# From: $obj_file" >> "$combined_asm"
            cat "$asm_file" >> "$combined_asm"
            echo "" >> "$combined_asm"

            # Analyze the disassembly for peripheral register accesses
            analyze_disassembly_for_peripherals "$asm_file" "$base_name" peripheral_accesses
        else
            log_warning "Failed to disassemble: $obj_file"
        fi
    done

    log_success "Combined disassembly saved to: $combined_asm"

    # Generate analysis results
    generate_peripheral_analysis_results peripheral_accesses "$build_type"
}

# Function to analyze disassembly for peripheral register accesses
analyze_disassembly_for_peripherals() {
    local asm_file="$1"
    local source_name="$2"
    local -n accesses_ref="$3"

    # Look for ARM Thumb-2 peripheral register access patterns
    # MIMXRT700 peripheral base addresses (from memory map)
    local -A peripheral_bases=(
        ["0x40000000"]="GPIO0"
        ["0x40001000"]="GPIO1"
        ["0x40002000"]="GPIO2"
        ["0x40003000"]="GPIO3"
        ["0x40004000"]="GPIO4"
        ["0x40005000"]="GPIO5"
        ["0x40020000"]="CLKCTL0"
        ["0x40021000"]="CLKCTL1"
        ["0x40022000"]="RSTCTL0"
        ["0x40023000"]="RSTCTL1"
        ["0x40024000"]="SYSCON0"
        ["0x40025000"]="SYSCON1"
        ["0x40134000"]="XSPI0"
        ["0x40135000"]="XSPI1"
        ["0x40136000"]="XSPI2"
    )

    # Track current function context
    local current_function=""

    # Scan assembly for memory access instructions
    while IFS= read -r line; do
        # Extract function name from section headers
        if [[ "$line" =~ ^[0-9a-fA-F]+[[:space:]]+\<([^>]+)\>: ]]; then
            current_function="${BASH_REMATCH[1]}"
            continue
        fi

        # Look for ARM Thumb-2 load/store instructions with register+offset addressing
        # Pattern: f8c1 0938 -> str.w r0, [r1, #0x938]
        # Pattern: f8d1 0934 -> ldr.w r0, [r1, #0x934]
        if [[ "$line" =~ ^[[:space:]]*[0-9a-fA-F]+:[[:space:]]+([0-9a-fA-F]{4}[[:space:]]+[0-9a-fA-F]{4})[[:space:]]+([a-z]+\.w)[[:space:]]+.*\[.*#0x([0-9a-fA-F]+)\] ]]; then
            local hex_bytes="${BASH_REMATCH[1]}"
            local instruction="${BASH_REMATCH[2]}"
            local offset="0x${BASH_REMATCH[3]}"

            # Check if this looks like a peripheral register offset
            local offset_val=$((0x${BASH_REMATCH[3]}))

            # XSPI peripheral register offsets (common ones)
            local peripheral=""
            local register_name=""

            if [[ $offset_val -ge 0x0 && $offset_val -le 0x1000 ]]; then
                case $offset_val in
                    0x0) register_name="MCR" ;;
                    0x4) register_name="IPCR" ;;
                    0x8) register_name="FLSHCR" ;;
                    0x20) register_name="AHBCR" ;;
                    0x50) register_name="AHBRXBUF0CR" ;;
                    0x160) register_name="IPRXFCR" ;;
                    0x16c) register_name="IPTXFCR" ;;
                    0x908) register_name="IPCMD" ;;
                    0x90c) register_name="IPCMD1" ;;
                    0x934) register_name="INTR" ;;
                    0x938) register_name="INTRCLR" ;;
                    *) register_name="REG_0x$(printf "%x" $offset_val)" ;;
                esac

                # Determine peripheral based on context and register pattern
                if [[ "$current_function" =~ XSPI || "$source_name" =~ xspi ]]; then
                    peripheral="XSPI"
                elif [[ "$current_function" =~ CLOCK || "$source_name" =~ clock ]]; then
                    peripheral="CLKCTL"
                elif [[ "$current_function" =~ GPIO || "$source_name" =~ gpio ]]; then
                    peripheral="GPIO"
                elif [[ "$current_function" =~ RESET || "$source_name" =~ reset ]]; then
                    peripheral="RSTCTL"
                else
                    peripheral="UNKNOWN"
                fi

                # Create access record
                local access_record="{\"peripheral\":\"$peripheral\",\"register\":\"$register_name\",\"offset\":\"$offset\",\"source\":\"$source_name\",\"function\":\"$current_function\",\"instruction\":\"$instruction\",\"hex\":\"$hex_bytes\"}"
                accesses_ref+=("$access_record")
            fi
        fi

        # Also look for simple register access patterns like: 6808 ldr r0, [r1]
        if [[ "$line" =~ ^[[:space:]]*[0-9a-fA-F]+:[[:space:]]+([0-9a-fA-F]{4})[[:space:]]+([a-z]+)[[:space:]]+.*\[r[0-9]+\] ]]; then
            local hex_bytes="${BASH_REMATCH[1]}"
            local instruction="${BASH_REMATCH[2]}"

            # This is a base register access - could be peripheral if in the right context
            if [[ "$current_function" =~ XSPI || "$source_name" =~ xspi ]]; then
                local access_record="{\"peripheral\":\"XSPI\",\"register\":\"BASE_REG\",\"offset\":\"0x0\",\"source\":\"$source_name\",\"function\":\"$current_function\",\"instruction\":\"$instruction\",\"hex\":\"$hex_bytes\"}"
                accesses_ref+=("$access_record")
            fi
        fi
    done < "$asm_file"
}

# Function to generate peripheral analysis results
generate_peripheral_analysis_results() {
    local -n accesses_ref="$1"
    local build_type="$2"

    local results_file="$ANALYSIS_OUTPUT_DIR/clang_${build_type}_peripheral_analysis.json"

    # Create JSON structure
    echo "{" > "$results_file"
    echo "  \"build_type\": \"$build_type\"," >> "$results_file"
    echo "  \"toolchain\": \"clang\"," >> "$results_file"
    echo "  \"timestamp\": \"$(date -Iseconds)\"," >> "$results_file"
    echo "  \"total_accesses\": ${#accesses_ref[@]}," >> "$results_file"
    echo "  \"peripheral_accesses\": [" >> "$results_file"

    # Add access records
    for i in "${!accesses_ref[@]}"; do
        echo "    ${accesses_ref[i]}" >> "$results_file"
        if [[ $i -lt $((${#accesses_ref[@]} - 1)) ]]; then
            echo "," >> "$results_file"
        else
            echo "" >> "$results_file"
        fi
    done

    echo "  ]" >> "$results_file"
    echo "}" >> "$results_file"

    log_success "Peripheral analysis results saved to: $results_file"
}



# Main analysis workflow
main() {
    log_info "Starting MIMXRT700 Clang peripheral analysis..."

    # Step 1: Analyze Clang debug build object files
    log_info "Step 1: Analyzing Clang debug build object files..."
    if [[ -d "$CLANG_DEBUG_DIR" ]]; then
        analyze_clang_object_files "$CLANG_DEBUG_DIR" "debug"
    else
        log_warning "Clang debug build directory not found: $CLANG_DEBUG_DIR"
    fi

    # Step 2: Analyze Clang release build object files (if available)
    log_info "Step 2: Analyzing Clang release build object files..."
    if [[ -d "$CLANG_RELEASE_DIR" ]]; then
        analyze_clang_object_files "$CLANG_RELEASE_DIR" "release"
    else
        log_warning "Clang release build directory not found: $CLANG_RELEASE_DIR"
    fi

    # Step 3: Combine analysis results from both builds
    log_info "Step 3: Combining analysis results..."

    local combined_results="$ANALYSIS_OUTPUT_DIR/clang_combined_analysis.json"

    # Use Python to combine JSON results from both debug and release builds
    python3 << EOF
import json
import os
import glob

combined_data = {
    "toolchain": "clang",
    "builds_analyzed": [],
    "total_accesses": 0,
    "peripheral_accesses": [],
    "summary": {"peripherals": {}}
}

# Find all analysis result files
analysis_files = glob.glob("$ANALYSIS_OUTPUT_DIR/clang_*_peripheral_analysis.json")

for file_path in analysis_files:
    try:
        with open(file_path, 'r') as f:
            data = json.load(f)

            build_type = data.get('build_type', 'unknown')
            combined_data['builds_analyzed'].append(build_type)

            if 'peripheral_accesses' in data:
                # Add build type to each access record
                for access in data['peripheral_accesses']:
                    access['build_type'] = build_type
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

with open("$combined_results", 'w') as f:
    json.dump(combined_data, f, indent=2)

print(f"Combined {len(analysis_files)} analysis files")
print(f"Total peripheral accesses found: {combined_data['total_accesses']}")
print(f"Builds analyzed: {', '.join(combined_data['builds_analyzed'])}")
EOF

    log_success "Combined analysis results saved to: $combined_results"

    # Step 4: Generate comprehensive summary report
    log_info "Step 4: Generating comprehensive analysis summary..."

    local summary_report="$ANALYSIS_OUTPUT_DIR/clang_analysis_summary.txt"

    python3 << EOF > "$summary_report"
import json

try:
    with open("$combined_results", 'r') as f:
        data = json.load(f)

    print("MIMXRT700 Clang Build Peripheral Analysis Summary")
    print("=" * 60)
    print()

    builds = data.get('builds_analyzed', [])
    total_accesses = data.get('total_accesses', 0)
    peripherals = data.get('summary', {}).get('peripherals', {})

    print(f"Toolchain: Clang/LLVM 19.1.6")
    print(f"Builds Analyzed: {', '.join(builds) if builds else 'None'}")
    print(f"Total Peripheral Register Accesses: {total_accesses}")
    print(f"Unique Peripherals Accessed: {len(peripherals)}")
    print()

    if peripherals:
        print("Peripheral Access Breakdown:")
        print("-" * 40)
        for peripheral, count in sorted(peripherals.items(), key=lambda x: x[1], reverse=True):
            print(f"  {peripheral:15} : {count:4} accesses")
        print()

    # Show detailed accesses for key peripherals
    key_peripherals = ['XSPI0', 'XSPI1', 'XSPI2', 'CLKCTL0', 'CLKCTL1', 'GPIO0', 'SYSCON0', 'SYSCON1']

    for peripheral in key_peripherals:
        accesses = [a for a in data.get('peripheral_accesses', []) if a.get('peripheral') == peripheral]
        if accesses:
            print(f"{peripheral} Register Accesses:")
            print("-" * 40)

            # Group by build type
            debug_accesses = [a for a in accesses if a.get('build_type') == 'debug']
            release_accesses = [a for a in accesses if a.get('build_type') == 'release']

            if debug_accesses:
                print(f"  Debug Build ({len(debug_accesses)} accesses):")
                for access in debug_accesses[:5]:  # Show first 5 accesses
                    reg = access.get('register', 'Unknown')
                    instr = access.get('instruction', 'Unknown')
                    source = access.get('source', 'Unknown')
                    print(f"    {reg} ({instr}) in {source}")
                if len(debug_accesses) > 5:
                    print(f"    ... and {len(debug_accesses) - 5} more debug accesses")

            if release_accesses:
                print(f"  Release Build ({len(release_accesses)} accesses):")
                for access in release_accesses[:5]:  # Show first 5 accesses
                    reg = access.get('register', 'Unknown')
                    instr = access.get('instruction', 'Unknown')
                    source = access.get('source', 'Unknown')
                    print(f"    {reg} ({instr}) in {source}")
                if len(release_accesses) > 5:
                    print(f"    ... and {len(release_accesses) - 5} more release accesses")
            print()

    if total_accesses == 0:
        print("Note: No peripheral register accesses were detected in the disassembly.")
        print("This could be due to:")
        print("  - High-level driver abstractions hiding direct register access")
        print("  - Compiler optimizations inlining register accesses")
        print("  - Register accesses through function pointers or computed addresses")
        print("  - Analysis limitations in detecting complex addressing patterns")

except Exception as e:
    print(f"Error generating summary: {e}")
EOF

    log_success "Analysis summary saved to: $summary_report"

    # Display summary
    echo ""
    echo "=============================================================="
    echo "CLANG BUILD PERIPHERAL ANALYSIS RESULTS"
    echo "=============================================================="
    cat "$summary_report"

    log_success "Clang peripheral analysis completed!"
    log_info "All results saved in: $ANALYSIS_OUTPUT_DIR"
}

# Execute main function
main "$@"
