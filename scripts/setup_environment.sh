#!/bin/bash

# Environment Setup Script for MIMXRT700 LLVM/Clang Build
# This script verifies prerequisites and sets up the build environment

set -e

echo "=== MIMXRT700 LLVM/Clang Environment Setup ==="

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to check version
check_cmake_version() {
    local version=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
    local major=$(echo $version | cut -d. -f1)
    local minor=$(echo $version | cut -d. -f2)
    
    if [ "$major" -gt 3 ] || ([ "$major" -eq 3 ] && [ "$minor" -ge 15 ]); then
        return 0
    else
        return 1
    fi
}

# Check prerequisites
echo "Checking prerequisites..."

# Check CMake
if command_exists cmake; then
    if check_cmake_version; then
        echo "✓ CMake $(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+') found"
    else
        echo "❌ CMake version 3.15+ required, found $(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')"
        exit 1
    fi
else
    echo "❌ CMake not found. Install with: sudo apt install cmake"
    exit 1
fi

# Check Ninja
if command_exists ninja; then
    echo "✓ Ninja $(ninja --version) found"
else
    echo "❌ Ninja not found. Install with: sudo apt install ninja-build"
    exit 1
fi

# Check ARM GCC
if command_exists arm-none-eabi-gcc; then
    echo "✓ ARM GCC $(arm-none-eabi-gcc --version | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+') found"
else
    echo "❌ ARM GCC not found. Install with: sudo apt install gcc-arm-none-eabi"
    exit 1
fi

# Check wget
if command_exists wget; then
    echo "✓ wget found"
else
    echo "❌ wget not found. Install with: sudo apt install wget"
    exit 1
fi

# Check tar
if command_exists tar; then
    echo "✓ tar found"
else
    echo "❌ tar not found. Install with: sudo apt install tar"
    exit 1
fi

# Check git
if command_exists git; then
    echo "✓ git found"
else
    echo "❌ git not found. Install with: sudo apt install git"
    exit 1
fi

# Check disk space
echo ""
echo "Checking system requirements..."

# Check available disk space (need at least 8GB)
available_space=$(df . | tail -1 | awk '{print $4}')
required_space=$((8 * 1024 * 1024))  # 8GB in KB

if [ "$available_space" -gt "$required_space" ]; then
    echo "✓ Sufficient disk space available ($(($available_space / 1024 / 1024))GB free, 8GB required)"
else
    echo "❌ Insufficient disk space. Need 8GB, have $(($available_space / 1024 / 1024))GB"
    exit 1
fi

# Check available RAM
total_ram=$(free -m | awk 'NR==2{print $2}')
if [ "$total_ram" -gt 3072 ]; then  # 3GB minimum
    echo "✓ Sufficient RAM available (${total_ram}MB total, 4GB recommended)"
else
    echo "⚠️  Limited RAM detected (${total_ram}MB). Build may be slow. 4GB+ recommended"
fi

# Check number of CPU cores
cpu_cores=$(nproc)
echo "✓ CPU cores available: $cpu_cores"

echo ""
echo "=== Environment Check Complete ==="
echo "All prerequisites satisfied!"
echo ""
echo "System Summary:"
echo "  - CMake: $(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')"
echo "  - Ninja: $(ninja --version)"
echo "  - ARM GCC: $(arm-none-eabi-gcc --version | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')"
echo "  - Disk Space: $(($available_space / 1024 / 1024))GB available"
echo "  - RAM: ${total_ram}MB"
echo "  - CPU Cores: $cpu_cores"
echo ""
echo "Ready to build LLVM/Clang toolchain!"
echo "Run: ./build_llvm_complete.sh [project_directory]"
