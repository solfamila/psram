# dsp_xaf_playback

## Overview
The dsp_xaf_playback application demonstrates audio processing using the DSP core,
the Xtensa Audio Framework (XAF) middleware library, and select Xtensa audio
codecs.

When the application is started, a shell interface is displayed on the terminal
that executes from the ARM application.  User can control this with shell
commands which are relayed via RPMsg-Lite IPC to the DSP where they are
processed and response is returned.

Type "help" to see the command list.

This demo contains two applications:
- cm33/ is the ARM application for the Cortex-M33 core
- dsp/ is the DSP application for the DSP core

The demo will combine both applications into one ARM image.
With this, the ARM core will load and start the DSP application on
startup. Pre-compiled DSP binary images are provided under dsp/binary/ directory.
If you make changes to the DSP application, rebuild the ARM application after building the DSP application.
If you plan to use MCUXpresso IDE for Cortex-M33 you will have to make sure that
the preprocessor symbol DSP_IMAGE_COPY_TO_RAM, found in IDE project settings,
is defined to the value 1 when building release configuration.

The DSP application can be built by the following tools:
Xtensa Xplorer or Xtensa C Compiler. Required tool versions can be found
in MCUXpresso SDK Release Notes for the board. Application for Cortex-M33 can be built by the other toolchains listed there.
The ARM application will power and clock the DSP, so it must be loaded prior to loading the DSP application.

In order to debug both the Cortex-M33 and DSP side of the application, please follow the instructions:
1. It is necessary to run the Cortex-M33 side first and stop the application before the DSP_Start function
2. Run the xt-ocd daemon with proper settings
3. Download and debug the DSP application

In order to get TRACE debug output from the XAF it is necessary to define XF_TRACE 1 in the project settings.
It is possible to save the TRACE output into RAM using DUMP_TRACE_TO_BUF 1 define on project level.
Please see the initalization of the TRACE function in the xaf_main_dsp.c file.
For more details see XAF documentation.

## Known issues
The "file stop" command doesn't stop the playback for some small files (with low sample rate).

## Supported Boards
- [EVK-MIMXRT595](../../_boards/evkmimxrt595/dsp_examples/xaf_playback/example_board_readme.md)
- [EVK-MIMXRT685](../../_boards/evkmimxrt685/dsp_examples/xaf_playback/example_board_readme.md)
- [MIMXRT685-AUD-EVK](../../_boards/mimxrt685audevk/dsp_examples/xaf_playback/example_board_readme.md)
- [MIMXRT700-EVK](../../_boards/mimxrt700evk/dsp_examples/xaf_playback/example_board_readme.md)
