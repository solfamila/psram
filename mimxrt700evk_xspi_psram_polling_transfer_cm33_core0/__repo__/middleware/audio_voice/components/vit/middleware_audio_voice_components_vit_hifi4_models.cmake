# Add set(CONFIG_USE_middleware_audio_voice_components_vit_hifi4_models true) in config.cmake to use this component

include_guard(GLOBAL)
message("${CMAKE_CURRENT_LIST_FILE} component is included.")

      target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
          ${CMAKE_CURRENT_LIST_DIR}/HIFI4/Lib
        )

  
