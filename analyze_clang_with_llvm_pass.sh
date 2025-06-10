#!/bin/bash

# MIMXRT700 Clang Build Analysis Using LLVM Peripheral Analysis Pass
# This script rebuilds the project with LLVM IR generation and applies the existing peripheral analysis pass

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_ROOT/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc"
ANALYSIS_OUTPUT_DIR="$PROJECT_ROOT/clang_llvm_pass_analysis"
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
echo "MIMXRT700 Clang Build Analysis Using LLVM Peripheral Pass"
echo "=============================================================="
echo ""

# Create analysis output directory
mkdir -p "$ANALYSIS_OUTPUT_DIR"
log_info "Analysis results will be saved to: $ANALYSIS_OUTPUT_DIR"

# Verify prerequisites
if [[ ! -f "$PERIPHERAL_ANALYZER" ]]; then
    log_error "Peripheral analyzer not found at: $PERIPHERAL_ANALYZER"
    log_info "Please build the LLVM analysis pass first"
    exit 1
fi

if [[ ! -f "$LLVM_INSTALL_DIR/bin/clang" ]]; then
    log_error "Clang not found at: $LLVM_INSTALL_DIR/bin/clang"
    exit 1
fi

log_success "Prerequisites verified"

# Function to rebuild project with LLVM IR generation
rebuild_with_llvm_ir() {
    local build_type="$1"
    local ir_build_dir="$BUILD_DIR/build_clang_${build_type}_ir"
    
    log_info "Rebuilding project with LLVM IR generation (${build_type})..."
    
    # Clean previous IR build
    rm -rf "$ir_build_dir"
    mkdir -p "$ir_build_dir"
    cd "$ir_build_dir"
    
    # Copy necessary configuration files
    cp "$PROJECT_ROOT/cmake/CMakeLists_clang.txt" CMakeLists.txt
    cp "$PROJECT_ROOT/cmake/clang_toolchain.cmake" .
    cp "$PROJECT_ROOT/cmake/flags_clang.cmake" .
    cp "$PROJECT_ROOT/cmake/config_clang.cmake" .
    cp ../config.cmake .
    cp ../*.ld .
    
    # Modify flags to generate LLVM IR
    cat >> flags_clang.cmake << 'EOF'

# Add LLVM IR generation flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -emit-llvm -S")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -emit-llvm -S")

# Override object file extension for LLVM IR
set(CMAKE_C_OUTPUT_EXTENSION ".ll")
set(CMAKE_CXX_OUTPUT_EXTENSION ".ll")
EOF
    
    # Configure with CMake
    log_info "Configuring CMake for LLVM IR generation..."
    if ! cmake -DCMAKE_TOOLCHAIN_FILE=clang_toolchain.cmake \
              -DCMAKE_BUILD_TYPE="$build_type" \
              -DSdkRootDirPath="$SDK_ROOT" \
              . > cmake_config.log 2>&1; then
        log_error "CMake configuration failed"
        log_info "Check log: $ir_build_dir/cmake_config.log"
        return 1
    fi
    
    # Build to generate LLVM IR files
    log_info "Building project to generate LLVM IR files..."
    if ! make -j4 > build.log 2>&1; then
        log_warning "Build failed, but LLVM IR files may have been generated"
        log_info "Check log: $ir_build_dir/build.log"
    fi
    
    # Find generated LLVM IR files
    local ir_files=()
    while IFS= read -r -d '' file; do
        ir_files+=("$file")
    done < <(find . -name "*.ll" -print0 2>/dev/null)
    
    log_info "Found ${#ir_files[@]} LLVM IR files"
    
    # Copy IR files to analysis directory for processing
    local ir_analysis_dir="$ANALYSIS_OUTPUT_DIR/${build_type}_ir_files"
    mkdir -p "$ir_analysis_dir"
    
    for ir_file in "${ir_files[@]}"; do
        local base_name=$(basename "$ir_file")
        cp "$ir_file" "$ir_analysis_dir/"
        log_info "Copied IR file: $base_name"
    done
    
    echo "${#ir_files[@]}" > "$ANALYSIS_OUTPUT_DIR/${build_type}_ir_count.txt"
    
    cd "$PROJECT_ROOT"
    return 0
}

# Function to apply LLVM peripheral analysis pass
apply_peripheral_analysis() {
    local build_type="$1"
    local ir_analysis_dir="$ANALYSIS_OUTPUT_DIR/${build_type}_ir_files"
    
    if [[ ! -d "$ir_analysis_dir" ]]; then
        log_warning "No IR files found for $build_type build"
        return 1
    fi
    
    log_info "Applying LLVM peripheral analysis pass to $build_type IR files..."
    
    local analysis_results="$ANALYSIS_OUTPUT_DIR/${build_type}_peripheral_analysis.json"
    local combined_accesses=()
    
    # Process each LLVM IR file with the peripheral analyzer
    for ir_file in "$ir_analysis_dir"/*.ll; do
        if [[ -f "$ir_file" ]]; then
            local base_name=$(basename "$ir_file" .ll)
            local file_analysis="$ANALYSIS_OUTPUT_DIR/${build_type}_${base_name}_analysis.json"
            
            log_info "Analyzing: $base_name"
            
            if "$PERIPHERAL_ANALYZER" "$ir_file" > "$file_analysis" 2>/dev/null; then
                log_success "Analysis completed for: $base_name"
                
                # Extract peripheral accesses from this file's analysis
                if [[ -f "$file_analysis" ]] && [[ -s "$file_analysis" ]]; then
                    # Add source file info to each access record
                    python3 << EOF
import json
import sys

try:
    with open("$file_analysis", 'r') as f:
        data = json.load(f)
    
    # Add source file information to each access
    if 'peripheral_accesses' in data:
        for access in data['peripheral_accesses']:
            access['source_file'] = "$base_name"
            access['build_type'] = "$build_type"
    
    with open("$file_analysis", 'w') as f:
        json.dump(data, f, indent=2)
        
except Exception as e:
    print(f"Error processing $file_analysis: {e}", file=sys.stderr)
EOF
                fi
            else
                log_warning "Analysis failed for: $base_name"
            fi
        fi
    done
    
    # Combine all analysis results
    log_info "Combining analysis results for $build_type build..."
    
    python3 << EOF
import json
import glob
import os

combined_data = {
    "build_type": "$build_type",
    "toolchain": "clang",
    "timestamp": "$(date -Iseconds)",
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
            combined_data['peripheral_accesses'].extend(data['peripheral_accesses'])
            
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

print(f"Combined analysis: {combined_data['total_accesses']} peripheral accesses found")
print(f"Peripherals: {', '.join(peripheral_counts.keys()) if peripheral_counts else 'None'}")
EOF
    
    log_success "Analysis results saved to: $analysis_results"
}

# Main execution
main() {
    log_info "Starting MIMXRT700 Clang peripheral analysis using LLVM pass..."
    
    # Step 1: Rebuild debug build with LLVM IR generation
    log_info "Step 1: Rebuilding debug build with LLVM IR generation..."
    if rebuild_with_llvm_ir "debug"; then
        apply_peripheral_analysis "debug"
    fi
    
    # Step 2: Rebuild release build with LLVM IR generation
    log_info "Step 2: Rebuilding release build with LLVM IR generation..."
    if rebuild_with_llvm_ir "release"; then
        apply_peripheral_analysis "release"
    fi
    
    # Step 3: Generate comprehensive report
    log_info "Step 3: Generating comprehensive analysis report..."
    
    local final_report="$ANALYSIS_OUTPUT_DIR/llvm_pass_analysis_report.txt"
    
    python3 << EOF > "$final_report"
import json
import glob

print("MIMXRT700 Clang Build Peripheral Analysis - LLVM Pass Results")
print("=" * 70)
print()

# Process both debug and release results
for build_type in ['debug', 'release']:
    analysis_file = f"$ANALYSIS_OUTPUT_DIR/{build_type}_peripheral_analysis.json"
    
    try:
        with open(analysis_file, 'r') as f:
            data = json.load(f)
        
        print(f"{build_type.upper()} BUILD ANALYSIS:")
        print("-" * 40)
        
        total_accesses = data.get('total_accesses', 0)
        peripherals = data.get('summary', {}).get('peripherals', {})
        
        print(f"Total Peripheral Accesses: {total_accesses}")
        print(f"Unique Peripherals: {len(peripherals)}")
        
        if peripherals:
            print("\\nPeripheral Breakdown:")
            for peripheral, count in sorted(peripherals.items(), key=lambda x: x[1], reverse=True):
                print(f"  {peripheral:15} : {count:4} accesses")
        
        # Show sample accesses
        accesses = data.get('peripheral_accesses', [])
        if accesses:
            print("\\nSample Peripheral Accesses:")
            for access in accesses[:10]:  # Show first 10
                peripheral = access.get('peripheral', 'Unknown')
                register = access.get('register', 'Unknown')
                function = access.get('function', 'Unknown')
                source = access.get('source_file', 'Unknown')
                print(f"  {peripheral}.{register} in {function} ({source})")
            
            if len(accesses) > 10:
                print(f"  ... and {len(accesses) - 10} more accesses")
        
        print()
        
    except FileNotFoundError:
        print(f"{build_type.upper()} BUILD: No analysis results found")
        print()
    except Exception as e:
        print(f"{build_type.upper()} BUILD: Error reading results - {e}")
        print()

print("Analysis completed using LLVM peripheral analysis pass")
EOF
    
    # Display the report
    echo ""
    echo "=============================================================="
    echo "LLVM PASS ANALYSIS RESULTS"
    echo "=============================================================="
    cat "$final_report"
    
    log_success "LLVM peripheral analysis pass completed!"
    log_info "All results saved in: $ANALYSIS_OUTPUT_DIR"
}

# Execute main function
main "$@"
