# Clang/LLVM Toolchain file for ARM Cortex-M33
# This file configures CMake to use LLVM/Clang for cross-compilation to ARM Cortex-M33

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Set ARM GCC sysroot for embedded libraries
# Using system-installed ARM GCC
set(ARM_GCC_SYSROOT "/usr/lib/arm-none-eabi")

# Configure Clang/LLVM toolchain for compilation, ARM GCC for linking
# Auto-detect LLVM installation or use environment variable
if(DEFINED ENV{LLVM_INSTALL_DIR})
    set(LLVM_INSTALL_DIR "$ENV{LLVM_INSTALL_DIR}")
else()
    # Default to workspace-relative path
    get_filename_component(WORKSPACE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
    set(LLVM_INSTALL_DIR "${WORKSPACE_DIR}/llvm-install")
endif()

# Using locally-built LLVM installation for C/C++ compilation
set(CMAKE_C_COMPILER "${LLVM_INSTALL_DIR}/bin/clang")
set(CMAKE_CXX_COMPILER "${LLVM_INSTALL_DIR}/bin/clang++")
# Use LLVM Clang for assembly to maintain consistent flags
set(CMAKE_ASM_COMPILER "${LLVM_INSTALL_DIR}/bin/clang")

# Use LLVM tools for binary manipulation
set(CMAKE_OBJCOPY "${LLVM_INSTALL_DIR}/bin/llvm-objcopy")
set(CMAKE_OBJDUMP "${LLVM_INSTALL_DIR}/bin/llvm-objdump")
set(CMAKE_SIZE "${LLVM_INSTALL_DIR}/bin/llvm-size")
set(CMAKE_AR "${LLVM_INSTALL_DIR}/bin/llvm-ar")
set(CMAKE_RANLIB "${LLVM_INSTALL_DIR}/bin/llvm-ranlib")

# Use LLVM linker for final linking
set(CMAKE_LINKER "${LLVM_INSTALL_DIR}/bin/ld.lld")

# Set target triple for ARM Cortex-M33
set(CMAKE_C_COMPILER_TARGET "arm-none-eabi")
set(CMAKE_CXX_COMPILER_TARGET "arm-none-eabi")
set(CMAKE_ASM_COMPILER_TARGET "arm-none-eabi")

# Common flags for all configurations
set(COMMON_FLAGS "-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16")
set(COMMON_FLAGS "${COMMON_FLAGS} --sysroot=${ARM_GCC_SYSROOT}")
set(COMMON_FLAGS "${COMMON_FLAGS} -I${ARM_GCC_SYSROOT}/include")

# Library paths for ARM Cortex-M33 with hard float
set(ARM_LIB_PATH "${ARM_GCC_SYSROOT}/newlib/thumb/v8-m.main+fp/hard")
set(GCC_LIB_PATH "/usr/lib/gcc/arm-none-eabi/13.2.1/thumb/v8-m.main+fp/hard")

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

