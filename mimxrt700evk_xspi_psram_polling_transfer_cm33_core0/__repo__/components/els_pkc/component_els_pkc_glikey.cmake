# Add set(CONFIG_USE_component_els_pkc_glikey true) in config.cmake to use this component

include_guard(GLOBAL)
message("${CMAKE_CURRENT_LIST_FILE} component is included.")

      target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
          ${CMAKE_CURRENT_LIST_DIR}/src/comps/mcuxClGlikey/src/mcuxClGlikey.c
        )

  
      target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
          ${CMAKE_CURRENT_LIST_DIR}/.
          ${CMAKE_CURRENT_LIST_DIR}/src/comps/mcuxClGlikey/inc
          ${CMAKE_CURRENT_LIST_DIR}/src/comps/mcuxClGlikey/inc/internal
        )

  
