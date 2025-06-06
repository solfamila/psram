# Add set(CONFIG_USE_middleware_multicore_erpc_port_freertos true) in config.cmake to use this component

include_guard(GLOBAL)
message("${CMAKE_CURRENT_LIST_FILE} component is included.")

      target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
          ${CMAKE_CURRENT_LIST_DIR}/erpc/erpc_c/port/erpc_port_freertos.cpp
          ${CMAKE_CURRENT_LIST_DIR}/erpc/erpc_c/port/erpc_threading_freertos.cpp
          ${CMAKE_CURRENT_LIST_DIR}/erpc/erpc_c/port/erpc_setup_extensions_freertos.cpp
        )

  
      target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
          ${CMAKE_CURRENT_LIST_DIR}/erpc/erpc_c/port
        )

  
