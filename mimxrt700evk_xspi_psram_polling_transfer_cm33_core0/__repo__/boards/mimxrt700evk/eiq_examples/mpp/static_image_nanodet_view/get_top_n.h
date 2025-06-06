/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

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

#ifndef _GET_TOP_N_H_
#define _GET_TOP_N_H_

#include "mpp_api_types.h"
#include <stddef.h>
#include <stdint.h>

typedef struct
{
  float score;
  int index;
} result_t;

void MODEL_GetTopN(const uint8_t* tensorData, uint32_t tensorSize, mpp_tensor_type_t tensorType,
                   size_t numResults, float threshold, result_t* topResults);

#endif // _GET_TOP_N_H_
