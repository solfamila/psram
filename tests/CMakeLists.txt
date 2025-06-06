# CMakeLists.txt for Unit Tests
# Unity testing framework integration for MIMXRT700 project

cmake_minimum_required(VERSION 3.10.0)

project(mimxrt700_tests)

# Find Unity testing framework
if(NOT DEFINED UNITY_ROOT)
    if(EXISTS "$ENV{HOME}/unity/src/unity.c")
        set(UNITY_ROOT "$ENV{HOME}/unity" CACHE PATH "Path to Unity testing framework")
    elseif(EXISTS "/opt/unity/src/unity.c")
        set(UNITY_ROOT "/opt/unity" CACHE PATH "Path to Unity testing framework")
    else()
        set(UNITY_ROOT "/opt/unity" CACHE PATH "Path to Unity testing framework")
    endif()
endif()

# Check if Unity is available
if(EXISTS "${UNITY_ROOT}/src/unity.c")
    message(STATUS "Unity found at: ${UNITY_ROOT}")
else()
    message(WARNING "Unity not found. Unit tests will not be built.")
    message(STATUS "To install Unity: git clone https://github.com/ThrowTheSwitch/Unity.git ${UNITY_ROOT}")
    return()
endif()

# Include Unity
include_directories(${UNITY_ROOT}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Add Unity source
set(UNITY_SOURCES
    ${UNITY_ROOT}/src/unity.c
)

# Test configuration
enable_testing()

# Include directories for project under test
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(SUT_DIR "${PROJECT_ROOT}/mimxrt700evk_xspi_psram_polling_transfer_cm33_core0")

include_directories(
    ${SUT_DIR}
    ${PROJECT_ROOT}
)

# Mock headers for embedded-specific functions
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mocks)

# Test source files
set(TEST_SOURCES
    unit/test_xspi_psram_ops.c
    unit/test_main.c
    mocks/mock_fsl_xspi.c
    mocks/mock_fsl_debug_console.c
)

# Create test executable
add_executable(run_tests
    ${UNITY_SOURCES}
    ${TEST_SOURCES}
    test_runner.c
)

# Compiler flags for testing
target_compile_definitions(run_tests PRIVATE
    UNITY_INCLUDE_CONFIG_H
    TESTING_BUILD
)

# Add test
add_test(NAME unit_tests COMMAND run_tests)

# Custom target for running tests
add_custom_target(test_run
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_tests
    DEPENDS run_tests
    COMMENT "Running unit tests"
)

# Test coverage (if gcov is available)
option(ENABLE_COVERAGE "Enable test coverage" OFF)
if(ENABLE_COVERAGE)
    target_compile_options(run_tests PRIVATE --coverage)
    target_link_options(run_tests PRIVATE --coverage)
endif()

# Valgrind support (if available)
find_program(VALGRIND_PROGRAM valgrind)
if(VALGRIND_PROGRAM)
    add_custom_target(test_valgrind
        COMMAND ${VALGRIND_PROGRAM} --leak-check=full --error-exitcode=1 ${CMAKE_CURRENT_BINARY_DIR}/run_tests
        DEPENDS run_tests
        COMMENT "Running tests with Valgrind"
    )
endif()

# Test report generation
add_custom_target(test_report
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_tests > test_results.txt 2>&1 || true
    DEPENDS run_tests
    COMMENT "Generating test report"
)
