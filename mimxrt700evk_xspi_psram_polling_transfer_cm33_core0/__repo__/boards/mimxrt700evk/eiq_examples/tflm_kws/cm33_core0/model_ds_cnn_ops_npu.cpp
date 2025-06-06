/*
 * Copyright 2021-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/kernels/neutron/neutron.h"

tflite::MicroOpResolver &MODEL_GetOpsResolver()
{
    static tflite::MicroMutableOpResolver<8> s_microOpResolver;

    s_microOpResolver.AddDepthwiseConv2D();
    s_microOpResolver.AddDequantize();
    s_microOpResolver.AddPad();
    s_microOpResolver.AddQuantize();
    s_microOpResolver.AddReshape();
    s_microOpResolver.AddSlice();
    s_microOpResolver.AddSoftmax();
    s_microOpResolver.AddCustom(tflite::GetString_NEUTRON_GRAPH(),
        tflite::Register_NEUTRON_GRAPH());

    return s_microOpResolver;
}
