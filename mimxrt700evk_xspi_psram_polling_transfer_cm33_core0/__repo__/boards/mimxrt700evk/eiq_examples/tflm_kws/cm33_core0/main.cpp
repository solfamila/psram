/*
 * Copyright 2020-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "board_init.h"
#include "demo_config.h"
#include "demo_info.h"
#include "fsl_debug_console.h"
#include "audio.h"
#include "model.h"
#include "output_postproc.h"
#include "timer.h"

int main(void)
{
    BOARD_Init();
    TIMER_Init();

    DEMO_PrintInfo();

    if (MODEL_Init() != kStatus_Success)
    {
        PRINTF("Failed initializing model" EOL);
        for (;;) {}
    }

    tensor_dims_t inputDims;
    tensor_type_t inputType;
    uint8_t* inputData = MODEL_GetInputTensorData(&inputDims, &inputType);

    tensor_dims_t outputDims;
    tensor_type_t outputType;
    uint8_t* outputData = MODEL_GetOutputTensorData(&outputDims, &outputType);

    while (1)
    {
        /* Expected tensor dimensions: [batches, frames, mfcc, channels] */
        if (AUDIO_GetSpectralSample(inputData, inputDims.data[1] * inputDims.data[2]) != kStatus_Success)
        {
            PRINTF("Failed retrieving input audio" EOL);
            for (;;) {}
        }

        auto startTime = TIMER_GetTimeInUS();
        MODEL_RunInference();
        auto endTime = TIMER_GetTimeInUS();

        MODEL_ProcessOutput(outputData, &outputDims, outputType, endTime - startTime);
    }
}
