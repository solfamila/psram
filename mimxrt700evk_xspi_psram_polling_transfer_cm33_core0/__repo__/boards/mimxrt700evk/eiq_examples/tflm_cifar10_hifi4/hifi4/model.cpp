/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.
   Copyright 2021-2023 NXP

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

/* File modified by NXP. Changes are described in file
   /middleware/eiq/tensorflow-lite/readme.txt in section "Release notes" */

#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "fsl_debug_console.h"
#include "model.h"
#include "model_data.h"

static const tflite::Model* s_model = nullptr;
static tflite::MicroInterpreter* s_interpreter = nullptr;

extern tflite::MicroOpResolver &MODEL_GetOpsResolver();

// An area of memory to use for input, output, and intermediate arrays.
// (Can be adjusted based on the model needs.)
#ifdef TENSORARENA_NONCACHE
static uint8_t s_tensorArena[kTensorArenaSize] __ALIGNED(16) __attribute__((section("NonCacheable")));
#else
static uint8_t s_tensorArena[kTensorArenaSize] __ALIGNED(16);
#endif

static uint32_t s_tensorArenaSizeUsed = 0;
status_t MODEL_Init(void)
{
    // Map the model into a usable data structure. This doesn't involve any
    // copying or parsing, it's a very lightweight operation.
    s_model = tflite::GetModel(model_data);
    if (s_model->version() != TFLITE_SCHEMA_VERSION)
    {
        PRINTF("Model provided is schema version %d not equal "
               "to supported version %d!\r\n",
               s_model->version(), TFLITE_SCHEMA_VERSION);
        return kStatus_Fail;
    }

    // Pull in only the operation implementations we need.
    // This relies on a complete list of all the ops needed by this graph.
    // NOLINTNEXTLINE(runtime-global-variables)
    tflite::MicroOpResolver &micro_op_resolver = MODEL_GetOpsResolver();

    // Build an interpreter to run the model with.
    static tflite::MicroInterpreter static_interpreter(
        s_model, micro_op_resolver, s_tensorArena, kTensorArenaSize);
    s_interpreter = &static_interpreter;
 
 // Allocate memory from the tensor_arena for the model's tensors.
    TfLiteStatus allocate_status = s_interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        PRINTF("AllocateTensors() failed!\r\n");
        return kStatus_Fail;
    }

    s_tensorArenaSizeUsed = s_interpreter->arena_used_bytes();
    PRINTF("Core/NPU Frequency: %d MHz\r\n", CLOCK_GetFreq(kCLOCK_CoreSysClk)/1000000);
    PRINTF("TensorArena Addr: 0x%x - 0x%x\r\n", s_tensorArena, s_tensorArena + kTensorArenaSize);
    PRINTF("TensorArena Size: Total 0x%x (%d B); Used 0x%x (%d B)\r\n" , kTensorArenaSize, kTensorArenaSize, s_tensorArenaSizeUsed, s_tensorArenaSizeUsed);
    PRINTF("Model Addr: 0x%x - 0x%x\r\n" , model_data, model_data + sizeof(model_data));
    PRINTF("Model Size: 0x%x (%d B)\r\n" , sizeof(model_data), sizeof(model_data));
    PRINTF("Total Size Used: %d B (Model (%d B) + TensorArena (%d B))\r\n" , (sizeof(model_data) + s_tensorArenaSizeUsed), sizeof(model_data), s_tensorArenaSizeUsed);

    return kStatus_Success;
}

status_t MODEL_RunInference(void)
{
    if (s_interpreter->Invoke() != kTfLiteOk)
    {
        PRINTF("Invoke failed!\r\n");
        return kStatus_Fail;
    }

    return kStatus_Success;
}

uint8_t* GetTensorData(TfLiteTensor* tensor, tensor_dims_t* dims, tensor_type_t* type)
{
    switch (tensor->type)
    {
        case kTfLiteFloat32:
            *type = kTensorType_FLOAT32;
            break;
        case kTfLiteUInt8:
            *type = kTensorType_UINT8;
            break;
        case kTfLiteInt8:
            *type = kTensorType_INT8;
            break;
        default:
            assert("Unknown input tensor data type!\r\n");
    };

    dims->size = tensor->dims->size;
    assert(dims->size <= MAX_TENSOR_DIMS);
    for (int i = 0; i < tensor->dims->size; i++)
    {
        dims->data[i] = tensor->dims->data[i];
    }

    return tensor->data.uint8;
}

uint8_t* MODEL_GetInputTensorData(tensor_dims_t* dims, tensor_type_t* type)
{
    TfLiteTensor* inputTensor = s_interpreter->input(0);

    return GetTensorData(inputTensor, dims, type);
}

uint8_t* MODEL_GetOutputTensorData(tensor_dims_t* dims, tensor_type_t* type)
{
    TfLiteTensor* outputTensor = s_interpreter->output(0);

    return GetTensorData(outputTensor, dims, type);
}

// Convert unsigned 8-bit image data to model input format in-place.
void MODEL_ConvertInput(uint8_t* data, tensor_dims_t* dims, tensor_type_t type)
{
    int size = dims->data[2] * dims->data[1] * dims->data[3];
    switch (type)
    {
        case kTensorType_UINT8:
            break;
        case kTensorType_INT8:
            for (int i = size - 1; i >= 0; i--)
            {
                reinterpret_cast<int8_t*>(data)[i] =
                    static_cast<int>(data[i]) - 127;
            }
            break;
        case kTensorType_FLOAT32:
            for (int i = size - 1; i >= 0; i--)
            {
                reinterpret_cast<float*>(data)[i] =
                    (static_cast<int>(data[i]) - MODEL_INPUT_MEAN) / MODEL_INPUT_STD;
            }
            break;
        default:
            assert("Unknown input tensor data type!\r\n");
    }
}

const char* MODEL_GetModelName(void)
{
    return MODEL_NAME;
}
