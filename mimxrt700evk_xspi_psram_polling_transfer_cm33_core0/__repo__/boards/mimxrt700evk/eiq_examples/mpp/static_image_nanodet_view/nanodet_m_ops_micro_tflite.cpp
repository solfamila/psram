/*
 * Copyright 2022-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Set the operations used in model Nanodet-M. This allows reducing the code size.
 * Important Notice: User may find the list of operations needed by its model using tool https://netron.app
 */

#include "mpp_config.h"

#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/kernels/softmax.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#if defined(APP_USE_NEUTRON16_MODEL) || defined(APP_USE_NEUTRON64_MODEL)
#include "tensorflow/lite/micro/kernels/neutron/neutron.h"
#endif

tflite::MicroOpResolver &MODEL_GetOpsResolver()
{
#ifdef APP_USE_NEUTRON64_MODEL
    static tflite::MicroMutableOpResolver<3> s_microOpResolver;

    s_microOpResolver.AddTranspose();
    s_microOpResolver.AddResizeBilinear();
    s_microOpResolver.AddCustom(tflite::GetString_NEUTRON_GRAPH(),
            tflite::Register_NEUTRON_GRAPH());
#else
    static tflite::MicroMutableOpResolver<13> s_microOpResolver;

    s_microOpResolver.AddConv2D();
    s_microOpResolver.AddDepthwiseConv2D();
    s_microOpResolver.AddPad();
    s_microOpResolver.AddLeakyRelu();
    s_microOpResolver.AddMaxPool2D();
    s_microOpResolver.AddConcatenation();
    s_microOpResolver.AddReshape();
    s_microOpResolver.AddTranspose();
    s_microOpResolver.AddSplit();
    s_microOpResolver.AddResizeBilinear();
    s_microOpResolver.AddAdd();
    s_microOpResolver.AddLogistic();
    s_microOpResolver.AddSlice();
#endif
    return s_microOpResolver;
}
