# CMakeLists.txt for MIMXRT700 Peripheral Analysis Pass Test Suite

cmake_minimum_required(VERSION 3.16)
project(PeripheralAnalysisPassTests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(LLVM REQUIRED CONFIG)
find_package(GTest REQUIRED)

# Add LLVM definitions and include directories
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})

# Separate LLVM components
llvm_map_components_to_libnames(llvm_libs 
    Core
    Support
    IRReader
    Analysis
    Passes
    TransformUtils
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${GTEST_INCLUDE_DIRS}
)

# Create test executable
add_executable(test_peripheral_analysis
    test_peripheral_analysis.cpp
    ../src/PeripheralAnalysisPass.cpp
)

# Link libraries
target_link_libraries(test_peripheral_analysis
    ${llvm_libs}
    ${GTEST_LIBRARIES}
    pthread
)

# Compiler flags
target_compile_options(test_peripheral_analysis PRIVATE
    -fno-rtti
    -fno-exceptions
    -Wall
    -Wextra
    -g
)

# Add test to CTest
enable_testing()
add_test(NAME PeripheralAnalysisPassTests COMMAND test_peripheral_analysis)

# Custom target for running tests with verbose output
add_custom_target(run_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_peripheral_analysis --gtest_output=xml:test_results.xml
    DEPENDS test_peripheral_analysis
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Peripheral Analysis Pass Tests"
)

# Custom target for running tests with detailed output
add_custom_target(run_tests_verbose
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_peripheral_analysis --gtest_output=xml:test_results.xml --gtest_print_time=1
    DEPENDS test_peripheral_analysis
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Peripheral Analysis Pass Tests (Verbose)"
)

# Print configuration info
message(STATUS "LLVM_VERSION: ${LLVM_VERSION}")
message(STATUS "LLVM_INCLUDE_DIRS: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM_DEFINITIONS: ${LLVM_DEFINITIONS}")
message(STATUS "GTEST_INCLUDE_DIRS: ${GTEST_INCLUDE_DIRS}")
message(STATUS "GTEST_LIBRARIES: ${GTEST_LIBRARIES}")
