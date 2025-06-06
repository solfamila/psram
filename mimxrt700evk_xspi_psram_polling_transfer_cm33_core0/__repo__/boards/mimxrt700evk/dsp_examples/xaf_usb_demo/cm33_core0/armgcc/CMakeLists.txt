# CROSS COMPILER SETTING
set(CMAKE_SYSTEM_NAME Generic)
cmake_minimum_required(VERSION 3.10.0)

# THE VERSION NUMBER
SET (MCUXPRESSO_CMAKE_FORMAT_MAJOR_VERSION 2)
SET (MCUXPRESSO_CMAKE_FORMAT_MINOR_VERSION 0)

include(ide_overrides.cmake OPTIONAL)

if(CMAKE_SCRIPT_MODE_FILE)
  message("${MCUXPRESSO_CMAKE_FORMAT_MAJOR_VERSION}")
  return()
endif()


set(CMAKE_STATIC_LIBRARY_PREFIX)
set(CMAKE_STATIC_LIBRARY_SUFFIX)

set(CMAKE_EXECUTABLE_LIBRARY_PREFIX)
set(CMAKE_EXECUTABLE_LIBRARY_SUFFIX)

# CURRENT DIRECTORY
set(ProjDirPath ${CMAKE_CURRENT_SOURCE_DIR})

set(EXECUTABLE_OUTPUT_PATH ${ProjDirPath}/${CMAKE_BUILD_TYPE})
set(LIBRARY_OUTPUT_PATH ${ProjDirPath}/${CMAKE_BUILD_TYPE})


project(dsp_xaf_usb_demo_cm33_core0)

enable_language(ASM)

set(MCUX_BUILD_TYPES flash_debug flash_release)

set(MCUX_SDK_PROJECT_NAME dsp_xaf_usb_demo_cm33_core0.elf)

if (NOT DEFINED SdkRootDirPath)
    SET(SdkRootDirPath ${ProjDirPath}/../../../../../..)
endif()

include(${ProjDirPath}/flags.cmake)

include(${ProjDirPath}/config.cmake)

add_executable(${MCUX_SDK_PROJECT_NAME} 
"${ProjDirPath}/../main.c"
"${ProjDirPath}/../main.h"
"${ProjDirPath}/../cmd.c"
"${ProjDirPath}/../cmd.h"
"${ProjDirPath}/../dsp_ipc.c"
"${ProjDirPath}/../dsp_ipc.h"
"${ProjDirPath}/../dsp_config.h"
"${ProjDirPath}/../srtm_config.h"
"${ProjDirPath}/../srtm_config_audio.h"
"${ProjDirPath}/../rpmsg_config.h"
"${ProjDirPath}/../usb_device_descriptor.h"
"${ProjDirPath}/../usb_device_descriptor.c"
"${ProjDirPath}/../usb_device_config.h"
"${ProjDirPath}/../usb_audio_config.h"
"${ProjDirPath}/../composite.h"
"${ProjDirPath}/../composite.c"
"${ProjDirPath}/../audio_unified.h"
"${ProjDirPath}/../audio_unified.c"
"${ProjDirPath}/../dsp_support.c"
"${ProjDirPath}/../dsp_support.h"
"${SdkRootDirPath}/boards/mimxrt700evk/dsp_examples/xaf_usb_demo/hifi4/binary/dsp_literal_release.bin"
"${SdkRootDirPath}/boards/mimxrt700evk/dsp_examples/xaf_usb_demo/hifi4/binary/dsp_text_release.bin"
"${SdkRootDirPath}/boards/mimxrt700evk/dsp_examples/xaf_usb_demo/hifi4/binary/dsp_data_release.bin"
"${ProjDirPath}/../incbin.S"
"${ProjDirPath}/../board.h"
"${ProjDirPath}/../board.c"
"${ProjDirPath}/../clock_config.h"
"${ProjDirPath}/../clock_config.c"
"${ProjDirPath}/../pin_mux.c"
"${ProjDirPath}/../pin_mux.h"
"${ProjDirPath}/../hardware_init.c"
"${ProjDirPath}/../app.h"
"${ProjDirPath}/../FreeRTOSConfig_Gen.h"
"${ProjDirPath}/../fsl_debug_console_conf.h"
"${ProjDirPath}/../fsl_os_abstraction_config.h"
"${ProjDirPath}/../mcux_config.h"
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE
    ${ProjDirPath}/..
    ${SdkRootDirPath}/boards/mimxrt700evk/dsp_examples/xaf_usb_demo/hifi4/binary
)

set_source_files_properties("${ProjDirPath}/../usb_device_config.h" PROPERTIES COMPONENT_CONFIG_FILE "middleware_usb_device_ehci_config_header")
set_source_files_properties("${ProjDirPath}/../FreeRTOSConfig_Gen.h" PROPERTIES COMPONENT_CONFIG_FILE "middleware_freertos-kernel_config")
set_source_files_properties("${ProjDirPath}/../fsl_os_abstraction_config.h" PROPERTIES COMPONENT_CONFIG_FILE "component_osa_template_config")

include(${SdkRootDirPath}/devices/MIMXRT798S/all_lib_device.cmake)

IF(NOT DEFINED TARGET_LINK_SYSTEM_LIBRARIES)  
    SET(TARGET_LINK_SYSTEM_LIBRARIES "-lm -lc -lgcc -lnosys")  
ENDIF()  

TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--start-group)

target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE ${TARGET_LINK_SYSTEM_LIBRARIES})

TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--end-group)

ADD_CUSTOM_COMMAND(TARGET ${MCUX_SDK_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_OBJCOPY}
-Obinary ${EXECUTABLE_OUTPUT_PATH}/${MCUX_SDK_PROJECT_NAME} ${EXECUTABLE_OUTPUT_PATH}/dsp_xaf_usb_demo.bin)

set_target_properties(${MCUX_SDK_PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES "output.map;${EXECUTABLE_OUTPUT_PATH}/dsp_xaf_usb_demo.bin")

# wrap all libraries with -Wl,--start-group -Wl,--end-group to prevent link order issue
group_link_libraries()

