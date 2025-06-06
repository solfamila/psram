# Add set(CONFIG_USE_middleware_eiq_tensorflow_lite_micro_cmsis_nn true) in config.cmake to use this component

include_guard(GLOBAL)
message("${CMAKE_CURRENT_LIST_FILE} component is included.")

      target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/cmsis_nn/depthwise_conv.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/cmsis_nn/pooling.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/cmsis_nn/fully_connected.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/cmsis_nn/add.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/cmsis_nn/mul.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/cmsis_nn/conv.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/cmsis_nn/softmax.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/cmsis_nn/unidirectional_sequence_lstm.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/cmsis_nn/svdf.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/sub.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/logistic.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/reduce.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/strided_slice.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/quantize.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/lstm_eval.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/reshape.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/leaky_relu.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/transpose_conv.cpp
          ${CMAKE_CURRENT_LIST_DIR}/tensorflow/lite/micro/kernels/pad.cpp
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/LSTMFunctions/arm_lstm_unidirectional_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/LSTMFunctions/arm_lstm_unidirectional_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/BasicMathFunctions/arm_elementwise_mul_acc_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/BasicMathFunctions/arm_elementwise_add_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/BasicMathFunctions/arm_elementwise_mul_s16_batch_offset.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/BasicMathFunctions/arm_elementwise_add_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/BasicMathFunctions/arm_elementwise_mul_s16_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/BasicMathFunctions/arm_elementwise_mul_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/BasicMathFunctions/arm_elementwise_mul_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/SVDFunctions/arm_svdf_get_buffer_sizes_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/SVDFunctions/arm_svdf_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/SVDFunctions/arm_svdf_state_s16_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConcatenationFunctions/arm_concatenation_s8_z.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConcatenationFunctions/arm_concatenation_s8_w.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConcatenationFunctions/arm_concatenation_s8_x.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConcatenationFunctions/arm_concatenation_s8_y.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ReshapeFunctions/arm_reshape_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_transpose_conv_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_convolve_wrapper_s4.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_nn_mat_mult_kernel_row_offset_s8_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_convolve_get_buffer_sizes_s4.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_convolve_wrapper_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_convolve_wrapper_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_convolve_get_buffer_sizes_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_depthwise_conv_s4.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_nn_mat_mult_kernel_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_convolve_s4.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_depthwise_conv_fast_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_convolve_1x1_s4.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_nn_mat_mult_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_depthwise_conv_wrapper_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_convolve_1x1_s4_fast.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_depthwise_conv_3x3_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_depthwise_conv_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_nn_mat_mult_kernel_s8_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_convolve_get_buffer_sizes_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_convolve_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_depthwise_conv_get_buffer_sizes_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_nn_mat_mult_kernel_s4_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_convolve_1_x_n_s4.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_convolve_1_x_n_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_depthwise_conv_wrapper_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_nn_depthwise_conv_s8_core.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_depthwise_conv_s8_opt.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_depthwise_conv_s4_opt.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_convolve_1x1_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_depthwise_conv_get_buffer_sizes_s4.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_convolve_1x1_s8_fast.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_depthwise_conv_wrapper_s4.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_depthwise_conv_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_transpose_conv_get_buffer_sizes_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_depthwise_conv_get_buffer_sizes_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ConvolutionFunctions/arm_convolve_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/FullyConnectedFunctions/arm_fully_connected_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/FullyConnectedFunctions/arm_vector_sum_s8_s64.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/FullyConnectedFunctions/arm_fully_connected_get_buffer_sizes_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/FullyConnectedFunctions/arm_fully_connected_s4.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/FullyConnectedFunctions/arm_fully_connected_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/FullyConnectedFunctions/arm_vector_sum_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/FullyConnectedFunctions/arm_fully_connected_get_buffer_sizes_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ActivationFunctions/arm_relu_q7.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ActivationFunctions/arm_relu_q15.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ActivationFunctions/arm_nn_activation_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/ActivationFunctions/arm_relu6_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_lstm_step_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_mat_mul_core_1x_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_q7_to_q15_with_offset.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_s8_to_s16_unordered_with_offset.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_mat_mult_nt_t_s8_s32.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_lstm_calculate_gate_s8_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_depthwise_conv_nt_t_padded_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_mat_mult_nt_t_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_mat_mul_core_4x_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_depthwise_conv_nt_t_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_mat_mul_core_1x_s4.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_vec_mat_mult_t_svdf_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_depthwise_conv_nt_t_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nntables.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_mat_mult_nt_t_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_mat_mult_nt_t_s4.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_lstm_step_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_vec_mat_mul_result_acc_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_vec_mat_mult_t_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_vec_mat_mult_t_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_lstm_calculate_gate_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_depthwise_conv_nt_t_s4.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_vec_mat_mul_result_acc_s8_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/NNSupportFunctions/arm_nn_vec_mat_mult_t_s4.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/SoftmaxFunctions/arm_softmax_u8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/SoftmaxFunctions/arm_nn_softmax_common_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/SoftmaxFunctions/arm_softmax_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/SoftmaxFunctions/arm_softmax_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/SoftmaxFunctions/arm_softmax_s8_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/PoolingFunctions/arm_max_pool_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/PoolingFunctions/arm_avgpool_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/PoolingFunctions/arm_avgpool_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/PoolingFunctions/arm_avgpool_get_buffer_sizes_s8.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/PoolingFunctions/arm_avgpool_get_buffer_sizes_s16.c
          ${CMAKE_CURRENT_LIST_DIR}/third_party/cmsis_nn/Source/PoolingFunctions/arm_max_pool_s8.c
        )

  
  if(CONFIG_USE_COMPONENT_CONFIGURATION)
  message("===>Import configuration from ${CMAKE_CURRENT_LIST_FILE}")

      target_compile_definitions(${MCUX_SDK_PROJECT_NAME} PUBLIC
                  -DCMSIS_NN
              )
  
  
  endif()

