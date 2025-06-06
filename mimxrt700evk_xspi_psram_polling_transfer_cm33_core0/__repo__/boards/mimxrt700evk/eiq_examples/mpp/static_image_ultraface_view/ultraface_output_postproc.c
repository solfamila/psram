/*
 * Copyright 2022-2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * The function that processes the tensor output of model Ultraface-slim 240x320 and
 * Ultraface-slim-ultraslim 128x128
 */

#include <stdio.h>
#include <math.h>
#include <float.h>
#include "ultraface_output_postproc.h"

#include "get_top_n.h"
#include "fsl_debug_console.h"
#include "mpp_config.h"
#include APP_TFLITE_ULTRAFACE_INFO

#include "mpp_api.h"

static const float  DETECTION_TRESHOLD = 60;       // (%)
#define EOL                      "\r\n"
static const float NMS_THRESH = 0.5f;
#define INT8_RANGE               255     // 127 - (-128) = 255
box_data g_boxes[ULTRAFACE_MAX_POINTS] = {0};

/* decode the output tensor and fill-in boxes above detection threshold.
 * returns the number of valid boxes.
 **/
static int decode_output_int8(const int8_t *predictions, box_data boxes[], int nb_box_max)
{
    // predictions are in an array [no_face_score, face_score, left, top, right, bottom]
    int cls_offset = 1;
    int reg_offset = 2;
    int nbbox = 0;
    box_data curr_box;

    const int threshold = DETECTION_TRESHOLD * INT8_RANGE / 100 + ULTRAFACE_OUTPUT_ZERO_POINT;
    int8_t score;

    /* initialize elements */
    for (int i = 0; i < ULTRAFACE_MAX_POINTS; i++)
    {
        score = predictions[cls_offset];
        if (score >= threshold)
        {
            curr_box.label  = 1;
            curr_box.score  = (score - ULTRAFACE_OUTPUT_ZERO_POINT) * ULTRAFACE_OUTPUT_SCALE;
            curr_box.left   = (int16_t) (((predictions[reg_offset]     - ULTRAFACE_OUTPUT_ZERO_POINT) * ULTRAFACE_OUTPUT_SCALE)  * (1.0f * ULTRAFACE_WIDTH));
            curr_box.top    = (int16_t) (((predictions[reg_offset + 1] - ULTRAFACE_OUTPUT_ZERO_POINT) * ULTRAFACE_OUTPUT_SCALE)  * (1.0f * ULTRAFACE_HEIGHT));
            curr_box.right  = (int16_t) (((predictions[reg_offset + 2] - ULTRAFACE_OUTPUT_ZERO_POINT) * ULTRAFACE_OUTPUT_SCALE)  * (1.0f * ULTRAFACE_WIDTH));
            curr_box.bottom = (int16_t) (((predictions[reg_offset + 3] - ULTRAFACE_OUTPUT_ZERO_POINT) * ULTRAFACE_OUTPUT_SCALE)  * (1.0f * ULTRAFACE_HEIGHT));
            nbbox = nms_insert_box(boxes, curr_box, nbbox, NMS_THRESH, nb_box_max);
        }
        cls_offset = cls_offset + 6;
        reg_offset = reg_offset + 6;
    }
    return nbbox;
}

int32_t ULTRAFACE_ProcessOutput(const mpp_inference_cb_param_t *inf_out, box_data* final_boxes, int nb_box_max)
{
	if (inf_out == NULL) {
		PRINTF("ERROR: ULTRAFACE_ProcessOutput parameter 'inf_out' is null pointer" EOL);
		return -1;
	}
	if (final_boxes == NULL) {
		PRINTF("ERROR: ULTRAFACE_ProcessOutput parameter 'final_boxes' is null pointer" EOL);
		return -1;
	}
    int8_t* preds_int = NULL;

    if (inf_out->inference_type == MPP_INFERENCE_TYPE_TFLITE) {
        preds_int = (int8_t * ) inf_out-> out_tensors[0]->data; /* [1, 4420, 6] matrix */
        if (preds_int == NULL) {
            PRINTF("ERROR: ULTRAFACE_ProcessOutput: preds_int NULL pointer" EOL);
            return -1;
        }
    } else {
        PRINTF("ERROR: ULTRAFACE_ProcessOutput: Undefined Inference Engine" EOL);
        return -1;
    }
    /* clear old data */
    memset(final_boxes, 0, nb_box_max*sizeof(box_data));

    decode_output_int8(preds_int, final_boxes, nb_box_max);

    return 0;
}
