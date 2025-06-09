# Configuration file for MIMXRT700 XSPI PSRAM project - Clang build
# This file is a modified version of the original config.cmake with components
# disabled that are not compatible with the simplified Clang build setup

# Core SDK components - keep enabled
set(CONFIG_USE_CMSIS_Include_core_cm true)
set(CONFIG_USE_device_MIMXRT798S_CMSIS true)
set(CONFIG_USE_device_MIMXRT798S_startup true)
set(CONFIG_USE_driver_cache_cache64 true)
set(CONFIG_USE_driver_clock true)
set(CONFIG_USE_driver_common true)
set(CONFIG_USE_driver_lpuart true)
set(CONFIG_USE_driver_xspi true)
set(CONFIG_USE_middleware_baremetal true)

# Components disabled for simplified Clang build
# These components have complex dependencies on FreeRTOS or advanced UART features
# that are not needed for the basic XSPI PSRAM functionality

# Debug console components - disabled to avoid FreeRTOS dependencies
set(CONFIG_USE_utility_debug_console_lite false)
set(CONFIG_USE_component_lpuart_adapter false)

# Serial manager components - disabled for simplicity
set(CONFIG_USE_component_serial_manager false)
set(CONFIG_USE_component_serial_manager_uart false)

# List components - disabled as they're not needed for this example
set(CONFIG_USE_component_lists false)

# Additional utility components that can be disabled
set(CONFIG_USE_utility_assert_lite false)

# Memory and system components - keep enabled as they're essential
set(CONFIG_USE_utilities_misc_utilities true)

# CMSIS DSP - can be enabled if needed for signal processing
set(CONFIG_USE_CMSIS_DSP_Include false)

# Notes for production builds:
# 
# 1. To re-enable debug console output:
#    - Set CONFIG_USE_utility_debug_console_lite to true
#    - Set CONFIG_USE_component_lpuart_adapter to true
#    - Remove or modify debug_console_stub.c
#    - Ensure proper UART initialization in your code
#
# 2. To enable FreeRTOS support:
#    - Add FreeRTOS-specific configuration options
#    - Update compiler flags to include -DSDK_OS_FREE_RTOS
#    - Include FreeRTOS source files in the build
#
# 3. For advanced features:
#    - Enable CONFIG_USE_component_serial_manager for advanced UART handling
#    - Enable CONFIG_USE_component_lists for linked list utilities
#    - Enable CONFIG_USE_CMSIS_DSP_Include for DSP functions
#
# 4. Memory optimization:
#    - Disable unused drivers to reduce code size
#    - Use CONFIG_USE_utilities_misc_utilities selectively
#    - Consider disabling floating-point support if not needed
