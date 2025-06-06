cmake_minimum_required ( VERSION 3.13.0 )
project ( "OTA unit test"
          VERSION 1.0.0
          LANGUAGES C )

# Allow the project to be organized into folders.
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

# Use C90 if not specified.
if( NOT DEFINED CMAKE_C_STANDARD )
    set( CMAKE_C_STANDARD 90 )
endif()
if( NOT DEFINED CMAKE_C_STANDARD_REQUIRED )
    set( CMAKE_C_STANDARD_REQUIRED ON )
endif()

# Do not allow in-source build.
if( ${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR} )
    message( FATAL_ERROR "In-source build is not allowed. Please build in a separate directory, such as ${PROJECT_SOURCE_DIR}/build." )
endif()

# Set global path variables.
get_filename_component(__MODULE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(MODULE_ROOT_DIR ${__MODULE_ROOT_DIR} CACHE INTERNAL "OTA repository root.")

# Configure options to always show in CMake GUI.
option( BUILD_CLONE_SUBMODULES
        "Set this to ON to automatically clone any required Git submodules. When OFF, submodules must be manually cloned."
        ON )

# Set output directories.
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )

# ====================== Coverity Analysis Configuration =======================

# Include filepaths for source and include.
include( ${MODULE_ROOT_DIR}/otaFilePaths.cmake )
include( ${MODULE_ROOT_DIR}/otaDependenyFilePaths.cmake )

# Target for Coverity analysis that builds the library.
add_library( coverity_analysis
    ${OTA_SOURCES}
    ${OTA_OS_POSIX_SOURCES}
    ${OTA_MQTT_SOURCES}
    ${OTA_HTTP_SOURCES}
    ${TINYCBOR_SOURCES}
    ${JSON_SOURCES} )

# Build OTA library target without custom config dependency
target_compile_definitions( coverity_analysis PUBLIC OTA_DO_NOT_USE_CUSTOM_CONFIG=1 )

# OTA include path.
target_include_directories( coverity_analysis PUBLIC
    ${OTA_INCLUDE_PUBLIC_DIRS}
    ${OTA_INCLUDE_OS_POSIX_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}/unit-test
    ${CMAKE_CURRENT_LIST_DIR}/unit-test-http )
target_include_directories( coverity_analysis PRIVATE
    ${OTA_INCLUDE_PRIVATE_DIRS}
    ${JSON_INCLUDE_PUBLIC_DIRS}
    ${TINYCBOR_INCLUDE_DIRS} )

# Build OTA Library target without the assert macro
target_compile_options(coverity_analysis PUBLIC -DNDEBUG )
# ============================ Test configuration ==============================

# Define a CMock resource path.
set( CMOCK_DIR ${MODULE_ROOT_DIR}/test/unit-test/CMock CACHE INTERNAL "CMock library source directory." )

# Include CMock build configuration.
include( unit-test/cmock_build.cmake )

# Check if the CMock source directory exists, and if not present, clone the submodule
# if BUILD_CLONE_SUBMODULES configuration is enabled.
if( NOT EXISTS ${CMOCK_DIR}/src )
    # Attempt to clone CMock.
    if( ${BUILD_CLONE_SUBMODULES} )
        clone_cmock()
    else()
        message( FATAL_ERROR "The required submodule CMock does not exist. Either clone it manually, or set BUILD_CLONE_SUBMODULES to 1 to automatically clone it during build." )
    endif()
endif()

# Add unit test and coverage configuration.

# Use CTest utility for managing test runs. This has to be added BEFORE
# defining test targets with add_test()
enable_testing()

# Add build targets for CMock and Unit, required for unit testing.
add_cmock_targets()

# Add function to enable CMock based tests and coverage.
include( ${MODULE_ROOT_DIR}/tools/cmock/create_test.cmake )

# Include build configuration for unit tests.
add_subdirectory( unit-test )

#  ==================== Coverage Analysis configuration ========================

# Add a target for running coverage on tests.
add_custom_target( coverage
    COMMAND ${CMAKE_COMMAND} -DCMOCK_DIR=${CMOCK_DIR}
    -P ${MODULE_ROOT_DIR}/tools/cmock/coverage.cmake
    DEPENDS cmock unity ota_utest ota_base64_utest ota_cbor_utest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
