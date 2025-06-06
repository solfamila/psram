# Add set(CONFIG_USE_component_flash_nor_xspi true) in config.cmake to use this component

include_guard(GLOBAL)
message("${CMAKE_CURRENT_LIST_FILE} component is included.")

      target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
          ${CMAKE_CURRENT_LIST_DIR}/nor/xspi/fsl_xspi_nor_flash.c
          ${CMAKE_CURRENT_LIST_DIR}/nor/fsl_sfdp_parser.c
        )

  
      target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
          ${CMAKE_CURRENT_LIST_DIR}/nor
          ${CMAKE_CURRENT_LIST_DIR}/nor/xspi
        )

  
