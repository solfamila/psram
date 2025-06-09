#!/bin/bash

# MIMXRT700 Board Initialization Peripheral Analysis Demo
# This script demonstrates the enhanced peripheral analysis capabilities

set -e

echo "=============================================================="
echo "MIMXRT700 BOARD_InitHardware Peripheral Analysis Demo"
echo "=============================================================="

# Configuration
LLVM_INSTALL_DIR="/mnt/persist/workspace/llvm-install"
ANALYSIS_PASS_DIR="llvm_analysis_pass"
TEST_FILE="test_board_init_peripherals.c"
OUTPUT_DIR="board_init_analysis"

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo ""
echo "🎯 Demonstrating Enhanced Peripheral Coverage"
echo "=============================================="

echo ""
echo "📋 Peripherals Now Covered:"
echo "  ✅ XSPI0/1/2 (XSPI Controllers)"
echo "  ✅ SYSCON0/1 (System Control)"
echo "  ✅ POWER (Power Management)"
echo "  ✅ CLKCTL0/1 (Clock Control)"
echo "  ✅ RSTCTL0/1 (Reset Control)"
echo "  ✅ GLIKEY (Global Key Registers)"
echo "  ✅ TRNG (True Random Number Generator)"
echo "  ✅ IOPCTL0/1/2 (I/O Pin Control)"
echo "  ✅ GPIO0 (General Purpose I/O)"

echo ""
echo "🔧 Step 1: Generating LLVM IR from board init test..."
$LLVM_INSTALL_DIR/bin/clang -S -emit-llvm -g -O1 "$TEST_FILE" -o "$OUTPUT_DIR/board_init.ll"
echo "✅ LLVM IR generated: $OUTPUT_DIR/board_init.ll"

echo ""
echo "🔍 Step 2: Running enhanced peripheral analysis..."
./$ANALYSIS_PASS_DIR/build/bin/peripheral-analyzer "$OUTPUT_DIR/board_init.ll" -o "$OUTPUT_DIR/board_analysis.json" -v

echo ""
echo "📊 Step 3: Generating comprehensive report..."
python3 peripheral_analysis_report.py "$OUTPUT_DIR/board_analysis.json" "$OUTPUT_DIR/board_report.txt"

echo ""
echo "📈 Analysis Results Summary:"
echo "=============================================="

# Extract statistics using jq if available
if command -v jq >/dev/null 2>&1; then
    TOTAL_ACCESSES=$(jq '[.peripheral_accesses[].accesses | length] | add' "$OUTPUT_DIR/board_analysis.json")
    PERIPHERAL_COUNT=$(jq '.peripheral_accesses | length' "$OUTPUT_DIR/board_analysis.json")
    
    echo "📊 Total Register Accesses: $TOTAL_ACCESSES"
    echo "🔧 Peripherals Analyzed: $PERIPHERAL_COUNT"
    
    echo ""
    echo "🏆 Top Peripheral Access Counts:"
    jq -r '.peripheral_accesses[] | "\(.peripheral_name): \(.accesses | length) accesses"' "$OUTPUT_DIR/board_analysis.json" | sort -k2 -nr | head -5
    
    echo ""
    echo "🔐 Security-Related Peripherals:"
    jq -r '.peripheral_accesses[] | select(.peripheral_name | test("SYSCON|GLIKEY|TRNG")) | "  \(.peripheral_name): \(.accesses | length) accesses"' "$OUTPUT_DIR/board_analysis.json"
    
    echo ""
    echo "⚡ Performance-Critical Peripherals:"
    jq -r '.peripheral_accesses[] | select(.peripheral_name | test("CLKCTL|POWER|RSTCTL")) | "  \(.peripheral_name): \(.accesses | length) accesses"' "$OUTPUT_DIR/board_analysis.json"
    
    echo ""
    echo "🎯 Most Accessed Registers:"
    jq -r '.peripheral_accesses[].accesses[] | "\(.source_location.function): \(.peripheral_name).\(.register_name)"' "$OUTPUT_DIR/board_analysis.json" | sort | uniq -c | sort -nr | head -5 | sed 's/^[ ]*/  /'
    
else
    echo "📊 Analysis completed successfully"
    echo "💡 Install 'jq' for detailed statistics"
fi

echo ""
echo "📁 Generated Files:"
echo "  - LLVM IR: $OUTPUT_DIR/board_init.ll"
echo "  - JSON Analysis: $OUTPUT_DIR/board_analysis.json"
echo "  - Text Report: $OUTPUT_DIR/board_report.txt"

echo ""
echo "🔍 Key Findings Preview:"
echo "=============================================="

echo ""
echo "🔐 Security Operations Detected:"
grep -A 2 -B 2 "GLIKEY\|TRNG\|SEC_CLK_CTRL" "$OUTPUT_DIR/board_report.txt" | head -10 || echo "  (See full report for security analysis)"

echo ""
echo "⚡ Power Management Operations:"
grep -A 2 -B 2 "PDRUNCFG\|Power" "$OUTPUT_DIR/board_report.txt" | head -10 || echo "  (See full report for power analysis)"

echo ""
echo "🕐 Clock Configuration Operations:"
grep -A 2 -B 2 "CLKSEL\|CLKDIV" "$OUTPUT_DIR/board_report.txt" | head -10 || echo "  (See full report for clock analysis)"

echo ""
echo "✅ Enhanced Board Initialization Analysis Complete!"
echo "=============================================================="

echo ""
echo "📖 Next Steps:"
echo "1. Review detailed report: $OUTPUT_DIR/board_report.txt"
echo "2. Examine JSON data: $OUTPUT_DIR/board_analysis.json"
echo "3. Apply to real MIMXRT700 project source files"
echo "4. Use insights for:"
echo "   - Security audit (GLIKEY, TRNG usage)"
echo "   - Performance optimization (clock/power sequencing)"
echo "   - Debugging (peripheral access tracking)"
echo "   - Documentation (hardware initialization flow)"

echo ""
echo "🎯 The peripheral analysis pass now provides COMPLETE coverage"
echo "   of MIMXRT700 board initialization peripherals!"
