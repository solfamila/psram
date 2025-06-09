cmake_minimum_required(VERSION 3.16)
project(PeripheralAnalysisPass)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM
set(LLVM_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../llvm-install" CACHE PATH "LLVM installation directory")

# Add LLVM to the module path
list(APPEND CMAKE_MODULE_PATH "${LLVM_INSTALL_DIR}/lib/cmake/llvm")

find_package(LLVM REQUIRED CONFIG PATHS ${LLVM_INSTALL_DIR})

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set the LLVM header and library paths
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Find the libraries that correspond to the LLVM components
llvm_map_components_to_libnames(llvm_libs
    support
    core
    irreader
    analysis
)

# Add include directories
include_directories(include)

# Add the pass library
add_library(PeripheralAnalysisPass MODULE
    src/PeripheralAnalysisPass.cpp
)

# Link against LLVM libraries
target_link_libraries(PeripheralAnalysisPass ${llvm_libs})

# Set compiler flags
target_compile_features(PeripheralAnalysisPass PRIVATE cxx_std_17)

# Disable RTTI to match LLVM
if(NOT LLVM_ENABLE_RTTI)
    target_compile_options(PeripheralAnalysisPass PRIVATE -fno-rtti)
endif()

# Set output directory
set_target_properties(PeripheralAnalysisPass PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Create a tool to run the analysis
add_executable(peripheral-analyzer
    tools/peripheral-analyzer.cpp
    src/PeripheralAnalysisPass.cpp
)

target_link_libraries(peripheral-analyzer ${llvm_libs})
target_compile_features(peripheral-analyzer PRIVATE cxx_std_17)

if(NOT LLVM_ENABLE_RTTI)
    target_compile_options(peripheral-analyzer PRIVATE -fno-rtti)
endif()

# Set output directory for the tool
set_target_properties(peripheral-analyzer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Installation
install(TARGETS PeripheralAnalysisPass
    LIBRARY DESTINATION lib
)

install(TARGETS peripheral-analyzer
    RUNTIME DESTINATION bin
)

install(FILES include/PeripheralAnalysisPass.h
    DESTINATION include
)
