/*
 * Copyright 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef MODELRUNNER_H
#define MODELRUNNER_H
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#include "fsl_debug_console.h"
#include <inttypes.h>
#include <stdlib.h>
#include "flash_opts.h"
struct nn_server {
    const char* model_name;
    char* params;
    size_t      model_size;
    size_t      kTensorArenaSize;
    char*       json_buffer;
    size_t      json_size;
    int*   input_dims_data;
    struct {
        int64_t run;
        int64_t decode;
        int64_t input;
        int64_t output;
    } timing;
    struct {
        int32_t num_outputs;
        char* name[256];
        char* type[256];
        int64_t timing[256];
        int32_t index[16];
        size_t bytes[16];
        char* data[16];
        char data_type[16][20];
        int32_t outputs_size;
        int* shape_data[16];
        int32_t shape_size[16];
        float scale[16];
        int32_t zero_point[16];
    } output;
    struct {
        char* name[16];
        size_t bytes[16];
        char* data[16];
        char data_type[16][20];
        const int32_t* shape_data[16];
        int32_t shape_size[16];
        int32_t inputs_size;
        float scale[16];
        int32_t zero_point[16];
        char* input_data[16];
    } input;
    uint8_t* image_upload_data;
    char* model_upload;
    int inference_count;
    bool model_flash_load;

    FlashConfig* flash_config;
	char* rem_mem;
    int64_t run_ns;
};

typedef struct nn_server NNServer;

int cmd_router(char* cmd, NNServer* server);
void parse_cmd(void* arg);
int modelrunner();
char* inference_results(NNServer* server, size_t* data_len, int outputs_idx[], int n_outputs);
char* model_info(NNServer* server, size_t* data_len);

int64_t os_clock_now();

#if defined(__cplusplus)
}
#endif /* __cplusplus */


#endif
