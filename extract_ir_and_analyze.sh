#!/bin/bash

# Extract LLVM IR from existing Clang builds and apply peripheral analysis pass
# This script works with the already successful Clang builds

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc"
ANALYSIS_OUTPUT_DIR="$PROJECT_ROOT/clang_ir_analysis"
LLVM_INSTALL_DIR="$PROJECT_ROOT/llvm-install"
PERIPHERAL_ANALYZER="$PROJECT_ROOT/llvm_analysis_pass/build/bin/peripheral-analyzer"

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
echo "Extract LLVM IR from Clang Builds and Apply Peripheral Analysis"
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

if [[ ! -f "$LLVM_INSTALL_DIR/bin/llvm-dis" ]]; then
    log_error "llvm-dis not found at: $LLVM_INSTALL_DIR/bin/llvm-dis"
    exit 1
fi

# Function to extract LLVM IR from object files
extract_ir_from_objects() {
    local build_type="$1"
    local clang_build_dir="$BUILD_DIR/build_clang_${build_type}"
    
    if [[ ! -d "$clang_build_dir" ]]; then
        log_warning "Clang $build_type build directory not found: $clang_build_dir"
        return 1
    fi
    
    log_info "Extracting LLVM IR from $build_type object files..."
    
    # Find object files
    local object_files=()
    while IFS= read -r -d '' file; do
        object_files+=("$file")
    done < <(find "$clang_build_dir" -name "*.obj" -print0 2>/dev/null)
    
    if [[ ${#object_files[@]} -eq 0 ]]; then
        log_warning "No object files found in $clang_build_dir"
        return 1
    fi
    
    log_info "Found ${#object_files[@]} object files"
    
    # Try to extract LLVM IR using llvm-dis (for bitcode objects)
    local ir_dir="$ANALYSIS_OUTPUT_DIR/${build_type}_ir"
    mkdir -p "$ir_dir"
    
    local ir_count=0
    for obj_file in "${object_files[@]}"; do
        local base_name=$(basename "$obj_file" .obj)
        local ir_file="$ir_dir/${base_name}.ll"
        
        # Try to disassemble if it's LLVM bitcode
        if "$LLVM_INSTALL_DIR/bin/llvm-dis" "$obj_file" -o "$ir_file" 2>/dev/null; then
            log_success "Extracted IR: $base_name.ll"
            ((ir_count++))
        else
            # Object file is not LLVM bitcode, skip
            continue
        fi
    done
    
    log_info "Extracted $ir_count LLVM IR files from $build_type build"
    echo "$ir_count" > "$ANALYSIS_OUTPUT_DIR/${build_type}_ir_count.txt"
    
    return 0
}

# Function to create LLVM IR from source files directly
create_ir_from_sources() {
    local build_type="$1"
    
    log_info "Creating LLVM IR directly from source files ($build_type)..."
    
    # Key source files to analyze
    local source_files=(
        "$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/xspi_psram_polling_transfer.c"
        "$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/xspi_psram_ops.c"
        "$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/board.c"
        "$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/clock_config.c"
    )
    
    local ir_dir="$ANALYSIS_OUTPUT_DIR/${build_type}_source_ir"
    mkdir -p "$ir_dir"
    
    # Set up compilation flags similar to the working Clang build
    local clang_flags=""
    clang_flags+="-target arm-none-eabi "
    clang_flags+="-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16 "
    clang_flags+="-fno-builtin -fshort-enums -fno-common -ffunction-sections -fdata-sections "
    clang_flags+="-ffreestanding -fno-builtin -mapcs -std=gnu99 "
    
    # Build type specific flags
    if [[ "$build_type" == "debug" ]]; then
        clang_flags+="-g -O0 -DDEBUG "
    else
        clang_flags+="-O2 -DNDEBUG "
    fi
    
    # Include paths (simplified)
    local include_flags=""
    include_flags+="-I$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0 "
    include_flags+="-I$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/CMSIS/Core/Include "
    include_flags+="-I$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S "
    include_flags+="-I$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/__repo__/devices/MIMXRT798S/drivers "
    
    # Defines
    local defines=""
    defines+="-DCPU_MIMXRT798SGFOA_cm33_core0 "
    defines+="-DCPU_MIMXRT798SGFOA "
    defines+="-DARM_MATH_CM33 "
    
    local ir_count=0
    
    # Generate LLVM IR for each source file
    for source_file in "${source_files[@]}"; do
        if [[ -f "$source_file" ]]; then
            local base_name=$(basename "$source_file" .c)
            local ir_file="$ir_dir/${base_name}.ll"
            
            log_info "Generating IR for: $base_name.c"
            
            # Generate LLVM IR with minimal dependencies
            if "$LLVM_INSTALL_DIR/bin/clang" \
                $clang_flags \
                $defines \
                $include_flags \
                -S -emit-llvm \
                -o "$ir_file" \
                "$source_file" \
                > "$ir_dir/${base_name}_compile.log" 2>&1; then
                
                log_success "Generated IR: $base_name.ll"
                ((ir_count++))
            else
                log_warning "Failed to generate IR for: $base_name.c"
                log_info "Check log: $ir_dir/${base_name}_compile.log"
            fi
        fi
    done
    
    log_info "Generated $ir_count LLVM IR files from source"
    echo "$ir_count" > "$ANALYSIS_OUTPUT_DIR/${build_type}_source_ir_count.txt"
    
    return 0
}

# Function to apply peripheral analysis to IR files
analyze_ir_files() {
    local build_type="$1"
    local ir_dir="$2"
    
    if [[ ! -d "$ir_dir" ]]; then
        log_warning "IR directory not found: $ir_dir"
        return 1
    fi
    
    log_info "Applying peripheral analysis to $build_type IR files..."
    
    local analysis_results="$ANALYSIS_OUTPUT_DIR/${build_type}_analysis_results.json"
    local total_accesses=0
    
    # Initialize combined results
    echo '{"build_type":"'$build_type'","peripheral_accesses":[],"summary":{"total_accesses":0,"peripherals":{}}}' > "$analysis_results"
    
    # Process each IR file
    for ir_file in "$ir_dir"/*.ll; do
        if [[ -f "$ir_file" ]]; then
            local base_name=$(basename "$ir_file" .ll)
            local file_analysis="$ANALYSIS_OUTPUT_DIR/${build_type}_${base_name}_analysis.json"
            
            log_info "Analyzing IR file: $base_name.ll"
            
            if "$PERIPHERAL_ANALYZER" "$ir_file" > "$file_analysis" 2>/dev/null; then
                log_success "Analysis completed for: $base_name.ll"
                
                # Check if analysis found anything
                if [[ -f "$file_analysis" ]] && [[ -s "$file_analysis" ]]; then
                    # Count accesses in this file
                    local file_accesses=$(python3 -c "
import json
try:
    with open('$file_analysis', 'r') as f:
        data = json.load(f)
    print(len(data.get('peripheral_accesses', [])))
except:
    print(0)
")
                    if [[ $file_accesses -gt 0 ]]; then
                        log_success "Found $file_accesses peripheral accesses in $base_name.ll"
                        ((total_accesses += file_accesses))
                    fi
                fi
            else
                log_warning "Analysis failed for: $base_name.ll"
            fi
        fi
    done
    
    # Combine all analysis results
    if [[ $total_accesses -gt 0 ]]; then
        log_info "Combining analysis results..."
        
        python3 << EOF
import json
import glob

combined_data = {
    "build_type": "$build_type",
    "toolchain": "clang",
    "total_accesses": 0,
    "peripheral_accesses": [],
    "summary": {"peripherals": {}}
}

# Find all analysis files for this build type
analysis_files = glob.glob("$ANALYSIS_OUTPUT_DIR/${build_type}_*_analysis.json")

for file_path in analysis_files:
    try:
        with open(file_path, 'r') as f:
            data = json.load(f)
            
        if 'peripheral_accesses' in data:
            for access in data['peripheral_accesses']:
                access['source_file'] = file_path.split('_')[-2]  # Extract filename
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

with open("$analysis_results", 'w') as f:
    json.dump(combined_data, f, indent=2)

print(f"Total peripheral accesses: {combined_data['total_accesses']}")
EOF
        
        log_success "Combined analysis saved to: $analysis_results"
    else
        log_info "No peripheral accesses found in $build_type build"
    fi
    
    return 0
}

# Main execution
main() {
    log_info "Starting LLVM IR extraction and peripheral analysis..."
    
    # Try both debug and release builds
    for build_type in "debug" "release"; do
        log_info "Processing $build_type build..."
        
        # First try to extract IR from existing object files
        if extract_ir_from_objects "$build_type"; then
            local ir_dir="$ANALYSIS_OUTPUT_DIR/${build_type}_ir"
            analyze_ir_files "$build_type" "$ir_dir"
        fi
        
        # Also try to create IR directly from sources
        if create_ir_from_sources "$build_type"; then
            local source_ir_dir="$ANALYSIS_OUTPUT_DIR/${build_type}_source_ir"
            analyze_ir_files "${build_type}_source" "$source_ir_dir"
        fi
    done
    
    # Generate final report
    log_info "Generating final analysis report..."
    
    local final_report="$ANALYSIS_OUTPUT_DIR/peripheral_analysis_report.txt"
    
    python3 << EOF > "$final_report"
import json
import glob

print("MIMXRT700 Clang Build Peripheral Analysis - LLVM Pass Results")
print("=" * 70)
print()

# Find all analysis result files
result_files = glob.glob("$ANALYSIS_OUTPUT_DIR/*_analysis_results.json")

if not result_files:
    print("No analysis results found.")
    print()
    print("This could indicate:")
    print("- Object files are not LLVM bitcode (compiled to native code)")
    print("- Source compilation failed due to missing headers")
    print("- No peripheral register accesses in the analyzed code")
else:
    for result_file in sorted(result_files):
        try:
            with open(result_file, 'r') as f:
                data = json.load(f)
            
            build_type = data.get('build_type', 'unknown')
            total_accesses = data.get('total_accesses', 0)
            peripherals = data.get('summary', {}).get('peripherals', {})
            
            print(f"{build_type.upper()} BUILD:")
            print("-" * 40)
            print(f"Total Peripheral Accesses: {total_accesses}")
            print(f"Unique Peripherals: {len(peripherals)}")
            
            if peripherals:
                print("\\nPeripheral Breakdown:")
                for peripheral, count in sorted(peripherals.items(), key=lambda x: x[1], reverse=True):
                    print(f"  {peripheral:15} : {count:4} accesses")
            
            print()
            
        except Exception as e:
            print(f"Error reading {result_file}: {e}")

print("Analysis completed using LLVM peripheral analysis pass")
EOF
    
    echo ""
    echo "=============================================================="
    echo "FINAL ANALYSIS REPORT"
    echo "=============================================================="
    cat "$final_report"
    
    log_success "Analysis completed!"
    log_info "All results saved in: $ANALYSIS_OUTPUT_DIR"
}

# Execute main function
main "$@"
