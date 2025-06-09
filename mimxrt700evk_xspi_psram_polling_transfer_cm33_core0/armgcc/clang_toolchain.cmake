# Clang/LLVM Toolchain file for ARM Cortex-M33
# This file configures CMake to use LLVM/Clang for cross-compilation to ARM Cortex-M33

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Set ARM GCC sysroot for embedded libraries
# Update this path to match your ARM GCC installation
set(ARM_GCC_SYSROOT "/opt/gcc-arm-none-eabi-10.3-2021.10/arm-none-eabi")

# Configure Clang/LLVM toolchain
# Update these paths to match your LLVM installation
set(CMAKE_C_COMPILER "/opt/llvm-19.1.6/bin/clang")
set(CMAKE_CXX_COMPILER "/opt/llvm-19.1.6/bin/clang++")
set(CMAKE_ASM_COMPILER "/opt/llvm-19.1.6/bin/clang")

# Set LLVM tools
set(CMAKE_OBJCOPY "/opt/llvm-19.1.6/bin/llvm-objcopy")
set(CMAKE_OBJDUMP "/opt/llvm-19.1.6/bin/llvm-objdump")
set(CMAKE_SIZE "/opt/llvm-19.1.6/bin/llvm-size")
set(CMAKE_AR "/opt/llvm-19.1.6/bin/llvm-ar")
set(CMAKE_RANLIB "/opt/llvm-19.1.6/bin/llvm-ranlib")

# Set target triple for ARM Cortex-M33
set(CMAKE_C_COMPILER_TARGET "arm-none-eabi")
set(CMAKE_CXX_COMPILER_TARGET "arm-none-eabi")
set(CMAKE_ASM_COMPILER_TARGET "arm-none-eabi")

# Common flags for all configurations
set(COMMON_FLAGS "-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16")
set(COMMON_FLAGS "${COMMON_FLAGS} --sysroot=${ARM_GCC_SYSROOT}")
set(COMMON_FLAGS "${COMMON_FLAGS} -I${ARM_GCC_SYSROOT}/include")

# Library paths for ARM Cortex-M33 with hard float
set(ARM_LIB_PATH "${ARM_GCC_SYSROOT}/lib/thumb/v8-m.main+fp/hard")
set(GCC_LIB_PATH "/opt/gcc-arm-none-eabi-10.3-2021.10/lib/gcc/arm-none-eabi/10.3.1/thumb/v8-m.main+fp/hard")

# Set initial compiler flags to help CMake's compiler tests pass
set(CMAKE_C_FLAGS_INIT "${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${COMMON_FLAGS}")

# Set initial linker flags
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld ${COMMON_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} -L${ARM_LIB_PATH}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} -L${GCC_LIB_PATH}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} -L${ARM_GCC_SYSROOT}/lib")

# Don't try to link during compiler testing (avoids issues with missing libraries)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Search paths for find_package and similar commands
set(CMAKE_FIND_ROOT_PATH ${ARM_GCC_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
