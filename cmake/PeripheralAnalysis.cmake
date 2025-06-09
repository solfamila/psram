# PeripheralAnalysis.cmake - Integration for Peripheral Analysis Pass
#
# This file provides functions to integrate the peripheral analysis pass
# into the MIMXRT700 project build system.

# Find the peripheral analysis pass
find_library(PERIPHERAL_ANALYSIS_PASS_LIB
    NAMES PeripheralAnalysisPass libPeripheralAnalysisPass
    PATHS ${CMAKE_SOURCE_DIR}/llvm_analysis_pass/build/lib
    NO_DEFAULT_PATH
)

find_program(PERIPHERAL_ANALYZER_TOOL
    NAMES peripheral-analyzer
    PATHS ${CMAKE_SOURCE_DIR}/llvm_analysis_pass/build/bin
    NO_DEFAULT_PATH
)

# Function to add peripheral analysis to a target
function(add_peripheral_analysis TARGET_NAME)
    if(NOT PERIPHERAL_ANALYZER_TOOL)
        message(WARNING "Peripheral analyzer tool not found. Skipping analysis for ${TARGET_NAME}")
        return()
    endif()
    
    # Get the target's output directory
    get_target_property(TARGET_BINARY_DIR ${TARGET_NAME} BINARY_DIR)
    if(NOT TARGET_BINARY_DIR)
        set(TARGET_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    
    # Define output files
    set(IR_FILE "${TARGET_BINARY_DIR}/${TARGET_NAME}.ll")
    set(ANALYSIS_OUTPUT "${TARGET_BINARY_DIR}/${TARGET_NAME}_peripheral_analysis.json")
    
    # Add custom command to generate LLVM IR
    add_custom_command(
        OUTPUT ${IR_FILE}
        COMMAND ${CMAKE_C_COMPILER} 
            -S -emit-llvm 
            -g 
            -I${CMAKE_SOURCE_DIR}/../../../CMSIS/Core/Include
            -I${CMAKE_SOURCE_DIR}/../../../devices/MIMXRT798S/drivers
            -I${CMAKE_SOURCE_DIR}/../../../devices/MIMXRT798S
            -I${CMAKE_SOURCE_DIR}/../../../components/uart
            -I${CMAKE_SOURCE_DIR}/../../../components/serial_manager
            -I${CMAKE_SOURCE_DIR}/../../../components/lists
            -I${CMAKE_SOURCE_DIR}/../../../utilities/assert
            -I${CMAKE_SOURCE_DIR}/../../../utilities/misc_utilities
            -I${CMAKE_SOURCE_DIR}/../../../utilities/debug_console
            -I${CMAKE_SOURCE_DIR}/../board
            -I${CMAKE_SOURCE_DIR}
            -DCPU_MIMXRT798SGFOA_cm33_core0
            -DCPU_MIMXRT798SGFOA
            -DARM_MATH_CM33
            -D__FPU_PRESENT=1U
            -D__ARM_FEATURE_CMSE=3U
            -mcpu=cortex-m33
            -mthumb
            -mfloat-abi=hard
            -mfpu=fpv5-sp-d16
            ${CMAKE_SOURCE_DIR}/*.c
            -o ${IR_FILE}
        DEPENDS ${CMAKE_SOURCE_DIR}/*.c
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating LLVM IR for ${TARGET_NAME}"
    )
    
    # Add custom command to run peripheral analysis
    add_custom_command(
        OUTPUT ${ANALYSIS_OUTPUT}
        COMMAND ${PERIPHERAL_ANALYZER_TOOL} ${IR_FILE} -o ${ANALYSIS_OUTPUT} -v
        DEPENDS ${IR_FILE} ${PERIPHERAL_ANALYZER_TOOL}
        COMMENT "Running peripheral analysis for ${TARGET_NAME}"
    )
    
    # Add custom target for the analysis
    add_custom_target(${TARGET_NAME}_peripheral_analysis
        DEPENDS ${ANALYSIS_OUTPUT}
        COMMENT "Peripheral analysis for ${TARGET_NAME}"
    )
    
    # Make the analysis depend on the main target
    add_dependencies(${TARGET_NAME}_peripheral_analysis ${TARGET_NAME})
    
    message(STATUS "Added peripheral analysis for target: ${TARGET_NAME}")
    message(STATUS "  IR file: ${IR_FILE}")
    message(STATUS "  Analysis output: ${ANALYSIS_OUTPUT}")
endfunction()

# Function to run analysis on existing IR files
function(run_peripheral_analysis_on_ir IR_FILE OUTPUT_FILE)
    if(NOT PERIPHERAL_ANALYZER_TOOL)
        message(WARNING "Peripheral analyzer tool not found")
        return()
    endif()
    
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${PERIPHERAL_ANALYZER_TOOL} ${IR_FILE} -o ${OUTPUT_FILE} -v
        DEPENDS ${IR_FILE} ${PERIPHERAL_ANALYZER_TOOL}
        COMMENT "Running peripheral analysis on ${IR_FILE}"
    )
    
    add_custom_target(peripheral_analysis_${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${OUTPUT_FILE}
        COMMENT "Peripheral analysis"
    )
endfunction()
