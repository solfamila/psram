# Add set(CONFIG_USE_middleware_edgefast_bluetooth_le_audio_vocs true) in config.cmake to use this component

include_guard(GLOBAL)
message("${CMAKE_CURRENT_LIST_FILE} component is included.")

      target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
          ${CMAKE_CURRENT_LIST_DIR}/source/impl/ethermind/audio/vocs.c
          ${CMAKE_CURRENT_LIST_DIR}/source/impl/ethermind/audio/vocs_client.c
        )

  
      target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
          ${CMAKE_CURRENT_LIST_DIR}/include/bluetooth/audio
          ${CMAKE_CURRENT_LIST_DIR}/source/impl/ethermind/audio
        )

  
