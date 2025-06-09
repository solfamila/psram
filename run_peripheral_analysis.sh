#!/bin/bash

# MIMXRT700 XSPI PSRAM Peripheral Analysis Workflow
# This script demonstrates the complete analysis workflow

set -e

echo "=========================================="
echo "MIMXRT700 Peripheral Analysis Workflow"
echo "=========================================="

# Configuration
LLVM_INSTALL_DIR="/mnt/persist/workspace/llvm-install"
ANALYSIS_PASS_DIR="llvm_analysis_pass"
TEST_FILE="test_peripheral_access.c"
OUTPUT_DIR="analysis_results"

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo ""
echo "Step 1: Verifying LLVM installation..."
if [ ! -f "$LLVM_INSTALL_DIR/bin/clang" ]; then
    echo "❌ LLVM not found at $LLVM_INSTALL_DIR"
    exit 1
fi
echo "✅ LLVM found: $($LLVM_INSTALL_DIR/bin/clang --version | head -1)"

echo ""
echo "Step 2: Verifying peripheral analysis pass..."
if [ ! -f "$ANALYSIS_PASS_DIR/build/bin/peripheral-analyzer" ]; then
    echo "❌ Peripheral analyzer not found. Building..."
    cd "$ANALYSIS_PASS_DIR/build"
    export LLVM_INSTALL_DIR="$LLVM_INSTALL_DIR"
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j4
    cd - > /dev/null
fi
echo "✅ Peripheral analyzer ready"

echo ""
echo "Step 3: Generating LLVM IR from test file..."
$LLVM_INSTALL_DIR/bin/clang -S -emit-llvm -g -O1 "$TEST_FILE" -o "$OUTPUT_DIR/test.ll"
echo "✅ LLVM IR generated: $OUTPUT_DIR/test.ll"

echo ""
echo "Step 4: Running peripheral analysis..."
./$ANALYSIS_PASS_DIR/build/bin/peripheral-analyzer "$OUTPUT_DIR/test.ll" -o "$OUTPUT_DIR/analysis.json" -v

echo ""
echo "Step 5: Generating human-readable report..."
python3 peripheral_analysis_report.py "$OUTPUT_DIR/analysis.json" "$OUTPUT_DIR/analysis_report.txt"

echo ""
echo "Step 6: Analysis Summary..."
echo "=========================================="

# Extract key statistics from JSON
if command -v jq >/dev/null 2>&1; then
    TOTAL_ACCESSES=$(jq '[.peripheral_accesses[].accesses | length] | add' "$OUTPUT_DIR/analysis.json")
    PERIPHERAL_COUNT=$(jq '.peripheral_accesses | length' "$OUTPUT_DIR/analysis.json")
    echo "📊 Total Register Accesses: $TOTAL_ACCESSES"
    echo "🔧 Peripherals Analyzed: $PERIPHERAL_COUNT"
    
    echo ""
    echo "📋 Peripheral Breakdown:"
    jq -r '.peripheral_accesses[] | "  \(.peripheral_name): \(.accesses | length) accesses"' "$OUTPUT_DIR/analysis.json"
else
    echo "📊 Analysis completed successfully"
    echo "💡 Install 'jq' for detailed statistics"
fi

echo ""
echo "📁 Output Files Generated:"
echo "  - LLVM IR: $OUTPUT_DIR/test.ll"
echo "  - JSON Results: $OUTPUT_DIR/analysis.json"
echo "  - Text Report: $OUTPUT_DIR/analysis_report.txt"

echo ""
echo "🎯 Analysis Complete!"
echo "=========================================="

echo ""
echo "📖 Quick Report Preview:"
echo "----------------------------------------"
head -20 "$OUTPUT_DIR/analysis_report.txt"
echo "..."
echo "(See full report in $OUTPUT_DIR/analysis_report.txt)"

echo ""
echo "🔍 JSON Sample:"
echo "----------------------------------------"
if command -v jq >/dev/null 2>&1; then
    jq '.peripheral_accesses[0].accesses[0]' "$OUTPUT_DIR/analysis.json"
else
    echo "First access entry:"
    head -50 "$OUTPUT_DIR/analysis.json" | tail -30
fi

echo ""
echo "✅ Peripheral analysis workflow completed successfully!"
echo ""
echo "Next steps:"
echo "1. Review the detailed report: $OUTPUT_DIR/analysis_report.txt"
echo "2. Examine JSON data: $OUTPUT_DIR/analysis.json"
echo "3. Apply analysis to your MIMXRT700 project source files"
echo "4. Use insights for security and performance optimization"
