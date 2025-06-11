#!/bin/bash

# MIMXRT700 Peripheral Analysis Pass Test Runner
# This script builds and runs the comprehensive test suite

set -e  # Exit on any error

echo "🧪 MIMXRT700 Peripheral Analysis Pass Test Suite"
echo "=" $(printf '=%.0s' {1..60})
echo "Testing critical issues:"
echo "1. XCACHE_DisableCache detection (currently missing!)"
echo "2. ARM_MPU_Enable value extraction (should be 0x00000007)"
echo "3. Execution order accuracy (XCACHE should be first, not MPU)"
echo "4. Register address mapping correctness"
echo "=" $(printf '=%.0s' {1..60})

# Check if we're in the right directory
if [ ! -f "test_peripheral_analysis.cpp" ]; then
    echo "❌ Error: Must be run from the tests directory"
    echo "   Current directory: $(pwd)"
    echo "   Expected files: test_peripheral_analysis.cpp, CMakeLists.txt"
    exit 1
fi

# Check for required dependencies
echo "🔍 Checking dependencies..."

# Check for LLVM
if ! command -v llvm-config &> /dev/null; then
    echo "❌ Error: LLVM not found. Please install LLVM development packages."
    echo "   Ubuntu/Debian: sudo apt install llvm-dev"
    echo "   CentOS/RHEL: sudo yum install llvm-devel"
    exit 1
fi

# Check for Google Test
if ! pkg-config --exists gtest; then
    echo "⚠️  Warning: Google Test not found via pkg-config"
    echo "   Attempting to find system installation..."
    
    if [ ! -f "/usr/include/gtest/gtest.h" ] && [ ! -f "/usr/local/include/gtest/gtest.h" ]; then
        echo "❌ Error: Google Test not found. Please install Google Test."
        echo "   Ubuntu/Debian: sudo apt install libgtest-dev"
        echo "   CentOS/RHEL: sudo yum install gtest-devel"
        echo "   Or build from source: https://github.com/google/googletest"
        exit 1
    fi
fi

echo "✅ Dependencies found"

# Create build directory
echo "🏗️  Setting up build environment..."
mkdir -p build
cd build

# Configure with CMake
echo "⚙️  Configuring build..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build the test suite
echo "🔨 Building test suite..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "❌ Build failed!"
    exit 1
fi

echo "✅ Build successful"

# Run the tests
echo ""
echo "🚀 Running Test Suite..."
echo "=" $(printf '=%.0s' {1..60})

./test_peripheral_analysis --gtest_output=xml:test_results.xml

TEST_EXIT_CODE=$?

echo ""
echo "=" $(printf '=%.0s' {1..60})

if [ $TEST_EXIT_CODE -eq 0 ]; then
    echo "✅ ALL TESTS PASSED!"
    echo ""
    echo "📊 Test Results Summary:"
    echo "   - Function detection coverage: VERIFIED"
    echo "   - ARM_MPU_Enable value extraction: VERIFIED"
    echo "   - Execution order accuracy: VERIFIED"
    echo "   - Register address mapping: VERIFIED"
    echo ""
    echo "🎯 The LLVM analysis pass is working correctly!"
else
    echo "❌ TESTS FAILED!"
    echo ""
    echo "🔍 Critical Issues Detected:"
    echo "   Please review the test output above for specific failures."
    echo ""
    echo "📋 Common Issues to Check:"
    echo "   1. Missing XCACHE_DisableCache function analysis"
    echo "   2. Incorrect ARM_MPU_Enable value calculation"
    echo "   3. Wrong execution order (MPU_CTRL should not be first)"
    echo "   4. Incorrect register address mappings"
    echo ""
    echo "🛠️  Next Steps:"
    echo "   1. Fix the identified issues in PeripheralAnalysisPass.cpp"
    echo "   2. Re-run the tests to verify fixes"
    echo "   3. Update the analysis pass implementation"
fi

echo ""
echo "📄 Detailed test results saved to: test_results.xml"
echo "🏁 Test run complete"

exit $TEST_EXIT_CODE
