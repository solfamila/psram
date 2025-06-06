# Add set(CONFIG_USE_component_els_pkc_examples_hmac true) in config.cmake to use this component

include_guard(GLOBAL)
message("${CMAKE_CURRENT_LIST_FILE} component is included.")

      target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
          ${CMAKE_CURRENT_LIST_DIR}/examples/mcuxClHmac/mcuxClHmac_Els_Oneshot_External_Key_example.c
          ${CMAKE_CURRENT_LIST_DIR}/examples/mcuxClHmac/mcuxClHmac_Sw_Multipart_example.c
          ${CMAKE_CURRENT_LIST_DIR}/examples/mcuxClHmac/mcuxClHmac_Sw_Oneshot_example.c
        )

  
      target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
          ${CMAKE_CURRENT_LIST_DIR}/.
          ${CMAKE_CURRENT_LIST_DIR}/examples/mcuxCsslFlowProtection/inc
          ${CMAKE_CURRENT_LIST_DIR}/examples/mcuxCsslMemory/inc
          ${CMAKE_CURRENT_LIST_DIR}/src/comps/mcuxClExample/inc
          ${CMAKE_CURRENT_LIST_DIR}/src/comps/mcuxClBuffer/inc
        )

  
