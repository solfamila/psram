cmake_minimum_required ( VERSION 3.13.0 )
project ( "Defender unit test"
          VERSION 1.0.0
          LANGUAGES C )

# Allow the project to be organized into folders.
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

# Use C90.
set( CMAKE_C_STANDARD 90 )
set( CMAKE_C_STANDARD_REQUIRED ON )

# Do not allow in-source build.
if( ${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR} )
    message( FATAL_ERROR "In-source build is not allowed. Please build in a separate directory, such as ${PROJECT_SOURCE_DIR}/build." )
endif()

# Set global path variable.
get_filename_component( __MODULE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE )
set( MODULE_ROOT_DIR ${__MODULE_ROOT_DIR} CACHE INTERNAL "Device Defender repository root." )
set( UNIT_TEST_DIR ${MODULE_ROOT_DIR}/test/unit-test CACHE INTERNAL "Device Defender unit test directory." )
set( UNITY_DIR ${UNIT_TEST_DIR}/Unity CACHE INTERNAL "Unity library source directory." )

# Configure options to always show in CMake GUI.
option( BUILD_CLONE_SUBMODULES
        "Set this to ON to automatically clone any required Git submodules. When OFF, submodules must be manually cloned."
        ON )

# Set output directories.
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )

#  ====================== Coverity Analysis Configuration ======================

# Include filepaths for source and include.
include( ${MODULE_ROOT_DIR}/defenderFilePaths.cmake )

# Target for Coverity analysis that builds the library.
add_library( coverity_analysis
             ${DEFENDER_SOURCES} )

# Device Defender public include path.
target_include_directories( coverity_analysis
                            PUBLIC
                            ${DEFENDER_INCLUDE_PUBLIC_DIRS}
                            "${CMAKE_CURRENT_LIST_DIR}/include" )

# Disable logging/assert() calls when building the Coverity analysis target
target_compile_options(coverity_analysis PUBLIC -DNDEBUG -DDISABLE_LOGGING )

#  ============================  Test Configuration ============================

# Include Unity build configuration.
include( unit-test/unity_build.cmake )

# Check if the Unity source directory exists. If it does not exist and the
# BUILD_CLONE_SUBMODULES configuration is enabled, clone the Unity submodule.
if( NOT EXISTS ${UNITY_DIR}/src )
    # Attempt to clone Unity.
    if( ${BUILD_CLONE_SUBMODULES} )
        clone_unity()
    else()
        message( FATAL_ERROR "The required submodule Unity does not exist. Either clone it manually, or set BUILD_CLONE_SUBMODULES to 1 to automatically clone it during build." )
    endif()
endif()

# Use CTest utility for managing test runs.
enable_testing()

# Add build target for Unity, required for unit testing.
add_unity_target()

# Add functions to enable Unity based tests and coverage.
include( ${MODULE_ROOT_DIR}/tools/unity/create_test.cmake )

# Include build configuration for unit tests.
add_subdirectory( unit-test )

#  ====================== Coverage Analysis configuration ======================

# Add a target for running coverage on tests.
add_custom_target( coverage
                   COMMAND ${CMAKE_COMMAND} -DUNITY_DIR=${UNITY_DIR}
                                            -P ${MODULE_ROOT_DIR}/tools/unity/coverage.cmake
                   DEPENDS unity defender_utest
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR} )
