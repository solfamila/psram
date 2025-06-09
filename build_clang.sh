#!/bin/bash

# Build script for MIMXRT700 XSPI PSRAM project using Clang/LLVM
# This script builds the project using the custom-built LLVM/Clang toolchain

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== MIMXRT700 XSPI PSRAM Build Script (Clang/LLVM) ===${NC}"

# Check if LLVM/Clang is installed
if [ ! -f "/opt/llvm-19.1.6/bin/clang" ]; then
    echo -e "${RED}Error: LLVM/Clang not found at /opt/llvm-19.1.6/bin/clang${NC}"
    echo "Please ensure LLVM/Clang is properly installed."
    exit 1
fi

echo -e "${GREEN}✓ Found LLVM/Clang installation${NC}"

# Verify Clang version
CLANG_VERSION=$(/opt/llvm-19.1.6/bin/clang --version | head -n1)
echo -e "${BLUE}Using: ${CLANG_VERSION}${NC}"

# Set build configuration
BUILD_TYPE=${1:-debug}
PROJECT_DIR="mimxrt700evk_xspi_psram_polling_transfer_cm33_core0/armgcc"
BUILD_DIR="${PROJECT_DIR}/build_clang_${BUILD_TYPE}"

echo -e "${YELLOW}Build Type: ${BUILD_TYPE}${NC}"
echo -e "${YELLOW}Project Directory: ${PROJECT_DIR}${NC}"
echo -e "${YELLOW}Build Directory: ${BUILD_DIR}${NC}"

# Clean previous build if requested
if [ "$2" = "clean" ]; then
    echo -e "${YELLOW}Cleaning previous build...${NC}"
    rm -rf "${BUILD_DIR}"
fi

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo -e "${BLUE}Configuring CMake with Clang...${NC}"

# Configure with CMake using Clang-specific CMakeLists.txt
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -DCMAKE_TOOLCHAIN_FILE=../flags_clang.cmake \
      -f ../CMakeLists_clang.txt \
      ..

echo -e "${BLUE}Building project...${NC}"

# Build the project
make -j$(nproc)

echo -e "${GREEN}=== Build completed successfully! ===${NC}"

# Show build artifacts
echo -e "${BLUE}Build artifacts:${NC}"
ls -la *.elf *.bin 2>/dev/null || echo "No artifacts found"

# Show memory usage if available
if [ -f "output.map" ]; then
    echo -e "${BLUE}Memory usage summary:${NC}"
    grep -A 10 "Memory Configuration" output.map || echo "Memory info not available"
fi

echo -e "${GREEN}Build script completed!${NC}"
echo -e "${YELLOW}To flash the firmware, use the generated .bin file${NC}"
