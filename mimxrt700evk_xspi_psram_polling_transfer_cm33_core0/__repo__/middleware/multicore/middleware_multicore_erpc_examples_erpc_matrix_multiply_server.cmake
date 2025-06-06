# Add set(CONFIG_USE_middleware_multicore_erpc_examples_erpc_matrix_multiply_server true) in config.cmake to use this component

include_guard(GLOBAL)
message("${CMAKE_CURRENT_LIST_FILE} component is included.")

      target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
          ${CMAKE_CURRENT_LIST_DIR}/erpc/examples/MCUXPRESSO_SDK/erpc_matrix_multiply/service/erpc_matrix_multiply_server.cpp
          ${CMAKE_CURRENT_LIST_DIR}/erpc/examples/MCUXPRESSO_SDK/erpc_matrix_multiply/service/erpc_matrix_multiply_interface.cpp
          ${CMAKE_CURRENT_LIST_DIR}/erpc/examples/MCUXPRESSO_SDK/erpc_matrix_multiply/service/c_erpc_matrix_multiply_server.cpp
        )

  
      target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
          ${CMAKE_CURRENT_LIST_DIR}/erpc/examples/MCUXPRESSO_SDK/erpc_matrix_multiply/service
        )

  
