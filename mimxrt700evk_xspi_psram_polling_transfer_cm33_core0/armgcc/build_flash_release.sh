#!/bin/sh
if [ -d "CMakeFiles" ];then rm -rf CMakeFiles; fi
if [ -f "Makefile" ];then rm -f Makefile; fi
if [ -f "build.ninja" ];then rm -f build.ninja; fi
if [ -f "cmake_install.cmake" ];then rm -f cmake_install.cmake; fi
if [ -f "CMakeCache.txt" ];then rm -f CMakeCache.txt; fi

# Set SDK_ROOT if not already set
if [ -z "$SDK_ROOT" ]; then
    SDK_ROOT="$(pwd)/../__repo__"
fi

# Use correct toolchain file path
TOOLCHAIN_FILE="$SDK_ROOT/tools/cmake_toolchain_files/armgcc.cmake"

echo "Using SDK_ROOT: $SDK_ROOT"
echo "Using toolchain file: $TOOLCHAIN_FILE"

cmake -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=flash_release -DSdkRootDirPath="$SDK_ROOT" .
make -j 2>&1 | tee build_log.txt
