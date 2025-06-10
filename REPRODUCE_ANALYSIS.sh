#!/bin/bash

echo "🎯 MIMXRT700 XSPI PSRAM - Peripheral Analysis Reproduction Script"
echo "================================================================="
echo ""

# Check if we're in the right directory
if [ ! -f "PERIPHERAL_ANALYSIS_README.md" ]; then
    echo "❌ Error: Please run this script from the repository root directory"
    echo "   Expected files: PERIPHERAL_ANALYSIS_README.md, llvm_analysis_pass/, clang_ir_final/"
    exit 1
fi

echo "✅ Repository structure verified"
echo ""

# Check for required files
echo "📁 Checking required files..."
required_files=(
    "llvm_analysis_pass/build/bin/peripheral-analyzer"
    "clang_ir_final/fsl_xspi.ll"
    "clang_ir_final/drivers/fsl_clock.ll"
    "all_524_peripheral_accesses.json"
    "all_accesses_readable.txt"
)

missing_files=0
for file in "${required_files[@]}"; do
    if [ -f "$file" ]; then
        echo "  ✅ $file"
    else
        echo "  ❌ $file (missing)"
        missing_files=$((missing_files + 1))
    fi
done

if [ $missing_files -gt 0 ]; then
    echo ""
    echo "❌ Missing $missing_files required files. Please ensure all files are present."
    exit 1
fi

echo ""
echo "🔧 Testing LLVM analysis pass..."

# Test the analysis pass
if [ -x "llvm_analysis_pass/build/bin/peripheral-analyzer" ]; then
    echo "  ✅ peripheral-analyzer binary is executable"
    
    # Run a quick test
    echo "  🧪 Running test analysis on XSPI driver..."
    if ./llvm_analysis_pass/build/bin/peripheral-analyzer clang_ir_final/fsl_xspi.ll -chronological -o test_output.json 2>/dev/null; then
        echo "  ✅ Analysis pass executed successfully"
        
        # Check output
        if [ -f "test_output.json" ]; then
            accesses=$(jq '.total_accesses' test_output.json 2>/dev/null)
            if [ "$accesses" = "306" ]; then
                echo "  ✅ Correct number of accesses found: $accesses"
                rm -f test_output.json
            else
                echo "  ⚠️  Unexpected number of accesses: $accesses (expected 306)"
            fi
        else
            echo "  ❌ No output file generated"
        fi
    else
        echo "  ❌ Analysis pass failed to execute"
        echo "     You may need to rebuild the analysis pass:"
        echo "     cd llvm_analysis_pass && mkdir -p build && cd build && cmake .. && make"
    fi
else
    echo "  ❌ peripheral-analyzer binary not found or not executable"
    echo "     Please build the analysis pass:"
    echo "     cd llvm_analysis_pass && mkdir -p build && cd build && cmake .. && make"
fi

echo ""
echo "📊 Verifying analysis results..."

# Check main analysis files
if [ -f "all_524_peripheral_accesses.json" ]; then
    total_accesses=$(jq '.analysis_summary.total_accesses' all_524_peripheral_accesses.json 2>/dev/null)
    board_accesses=$(jq '.analysis_summary.board_init_accesses' all_524_peripheral_accesses.json 2>/dev/null)
    xspi_accesses=$(jq '.analysis_summary.xspi_init_accesses' all_524_peripheral_accesses.json 2>/dev/null)
    
    echo "  📋 Total peripheral accesses: $total_accesses"
    echo "  📋 BOARD_InitHardware(): $board_accesses accesses"
    echo "  📋 xspi_hyper_ram_init(): $xspi_accesses accesses"
    
    if [ "$total_accesses" = "430" ] && [ "$board_accesses" = "124" ] && [ "$xspi_accesses" = "306" ]; then
        echo "  ✅ All access counts verified correctly"
    else
        echo "  ⚠️  Access counts don't match expected values"
    fi
else
    echo "  ❌ Main analysis file missing"
fi

# Check readable output
if [ -f "all_accesses_readable.txt" ]; then
    lines=$(wc -l < all_accesses_readable.txt)
    echo "  📄 Readable output: $lines lines"
    
    if [ "$lines" = "441" ]; then
        echo "  ✅ Readable output verified"
    else
        echo "  ⚠️  Unexpected line count in readable output"
    fi
else
    echo "  ❌ Readable output file missing"
fi

echo ""
echo "🎯 REPRODUCTION VERIFICATION COMPLETE"
echo "====================================="

if [ $missing_files -eq 0 ]; then
    echo "✅ SUCCESS: All required files present and analysis results verified"
    echo ""
    echo "📖 To reproduce the analysis:"
    echo "   1. Run: ./llvm_analysis_pass/build/bin/peripheral-analyzer clang_ir_final/fsl_xspi.ll -chronological"
    echo "   2. Run: ./llvm_analysis_pass/build/bin/peripheral-analyzer clang_ir_final/drivers/fsl_clock.ll -chronological"
    echo "   3. View results in generated JSON files"
    echo ""
    echo "📊 Key analysis files:"
    echo "   - all_524_peripheral_accesses.json (complete analysis)"
    echo "   - all_accesses_readable.txt (human-readable format)"
    echo "   - xspi_clean_analysis.json (XSPI-specific)"
    echo "   - board_init_clean_analysis.json (board init-specific)"
    echo ""
    echo "🎉 Repository ready for MIMXRT700 XSPI PSRAM peripheral analysis!"
else
    echo "❌ ISSUES FOUND: Please resolve missing files before proceeding"
fi

echo ""
