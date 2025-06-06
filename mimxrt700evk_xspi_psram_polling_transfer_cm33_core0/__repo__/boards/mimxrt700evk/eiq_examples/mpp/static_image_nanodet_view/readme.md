# static_image_nanodet_view

## Overview

This example shows how to use the library to create one object detection using a file as source.

The machine learning framework used is TensorFlow Lite Micro.
The object detection model used is quantized Nanodet M model that detects multiple objects in an input image.
The model has 80 classes.

## Toolchains supported
- MCUXpresso, version 11.10.0
- GCC Arm Embedded, version 13.2.Rel1

## Hardware requirements
Refer to board.readme for hardware setup requirements.
- [MIMXRT700-EVK](../../../_boards/mimxrt700evk/eiq_examples/mpp/board_readme.md)
- [MIMXRT1170-EVKB](../../../_boards/evkbmimxrt1170/eiq_examples/mpp/board_readme.md)

## Use-case Description

### High-level description
```

                                                                   +---------------------------------------------------------+
                                                                   |                                                         |
                                                                   |                                                         |
                                                                  \ /                                                        |
                  +-------------+      +-------------+      +-------------+      +-------------+       +-------------+       |
                  |    static   |      |             |      |             |      |             |       |             |       |
Pipeline 0        |     image   | -->  |  2D convert | -->  |   labeled   | -->  | 2D convert  |  -->  |    Display  |       |
                  |             |  |   |(color+scale)|      |  rectangle  |      | (rotation)  |       |             |       |
                  +-------------+  |   +-------------+      +-------------+      +-------------+       +-------------+       |
                                   |                                                                                         |
                                   |     +-------------+      +--------------+      +-------------+                          |
                                   |     |             |      |              |      |             |                          |
Pipeline 1                         +---> |  2D convert | -->  | ML Inference | -->  |  NULL sink  |                          |
                                         |             |      |              |      |             |                          |
                                         +-------------+      +--------------+      +-------------+                          |
                                                                       |                                                     |
                                                                       |                                                     |
    +-----------------+                                                |                                                     |
    |  Main app:      |                                                |                                                     |
    | ML output       |   <----- ML Inference output callback ---------+                                                     |
    | post processing |                                                                                                      |
    |                 |   ------ Adding detected labeled rectangles ---------------------------------------------------------+
    +-----------------+
```

### Detailed description

Application creates two pipelines:

- One pipeline that runs the static image preview.
- Another pipeline that runs the ML inference on the static image.
- Pipeline 1 is split from pipeline 0
- Pipeline 0 executes the processing of each element sequentially and CANNOT be preempted by another pipeline.
- Pipeline 1 executes the processing of each element sequentially but CAN be preempted.

### Pipelines elements description

* Static image 320x320 BGRA format
* Display element is configured for a specific pixel format and resolution (board dependent)
* 2D convert element on pipeline 0 is configured to perform:
  - color space conversion from ARGB to the display pixel format
  - Scaling from 320x320 to the display resolution

* 2D convert element on pipeline 1 is configured to perform:
  - color space conversion from ARGB to RGB888
  - cropping to maintain image aspect ratio
  - scaling to 128x128 as mandated by the object detection model
  - rotation depending on the display orientation compared to landscape mode (NB: Rotation should be performed 
  after the labeled-rectangle to get labels in the right orientation).

* The labeled rectangle element draws a crop window from which the static image is sent to
  the ML inference element. The labeled rectangle element also displays the labels and boxes of the detected objects.
* The ML inference element runs an inference on the image pre-processed by the 2D convert element.
* The NULL sink element closes pipeline 1 (in MPP concept, only sink elements can close a pipeline).

* At every inference, the ML inference element invokes a callback containing the inference outputs.
These outputs are post-processed by the callback client component(in this case, the main task of the application) 
that determinates the boxes of detected objects and performs NMS to pick the best box for each detected object.

## Running the demo

EXPECTED OUTPUTS:
The expected outputs of the example are:
- For each detected object, a labeled rectangle should be displayed on the screen
- Logs below should be displayed on the debug console

Logs for static_image_nanodet_m_view example using TensorFlow Lite Micro model should look like this:
```
Inference Engine: TensorFlow-Lite Micro
Element stats --------------------------
nanodet : exec_time 716 ms
nanodet : box 0 label person score 82(%)
Element stats --------------------------
nanodet : exec_time 718 ms
nanodet : box 0 label person score 82(%)
Element stats --------------------------
nanodet : exec_time 716 ms
nanodet : box 0 label person score 82(%)
Element stats --------------------------
nanodet : exec_time 717 ms
nanodet : box 0 label person score 82(%)
```
## Important notes

TensorFLow Lite Micro is an optional engine for the ML Inference component of MPP.
This project embeds NXP's custom TensorFlow Lite Micro code by default.
TensorFLow Lite allows short-listing the "Operations" used by a specific model in order to reduce the binary image footprint.
This is done by implementing the function:

tflite::MicroOpResolver &MODEL_GetOpsResolver()

This example implements its own function MODEL_GetOpsResolver dedicated to Nanodet.
User may provide its own implementation of MODEL_GetOpsResolver when using a different model.
