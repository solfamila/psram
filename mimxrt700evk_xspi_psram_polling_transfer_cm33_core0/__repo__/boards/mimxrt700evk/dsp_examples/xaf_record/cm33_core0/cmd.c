/*
 * Copyright 2019-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*${header:start}*/
#include "cmd.h"
#include "board.h"
#include "dsp_ipc.h"

#include <string.h>
#include <stdint.h>
#include "fsl_debug_console.h"
#include "fsl_shell.h"

/* Enable VIT models*/
#if (XA_VIT_PRE_PROC == 1)
#include "PL_platformTypes_HIFI_FUSIONF1.h"
#if (VIT_MODELS_ALL == 1)
#include "VIT_Model_en.h"
#include "VIT_Model_cn.h"
#include "VIT_Model_de.h"
#include "VIT_Model_es.h"
#include "VIT_Model_fr.h"
#include "VIT_Model_it.h"
#include "VIT_Model_ja.h"
#include "VIT_Model_ko.h"
#include "VIT_Model_tr.h"
#include "VIT_Model_pt.h"
#else
#include "VIT_Model_en.h"
#endif
#endif
/*${header:end}*/

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*${macro:start}*/

/*${macro:end}*/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*${prototype:start}*/
static void initMessage(srtm_message *msg);

static shell_status_t shellEcho(shell_handle_t shellHandle, int32_t argc, char **argv);

#if XA_PCM_GAIN
static shell_status_t shellRecDMIC(shell_handle_t shellHandle, int32_t argc, char **argv);
#endif

/*${prototype:end}*/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*${variable:start}*/
SHELL_COMMAND_DEFINE(version, "\r\n\"version\": Query DSP for component versions\r\n", shellEcho, 0);

#if XA_PCM_GAIN
SHELL_COMMAND_DEFINE(
    record_dmic,
    "\r\n\"record_dmic\": Record DMIC audio"
#if (XA_VIT_PRE_PROC == 1)
    " , perform voice recognition (VIT)"
#endif
    " and playback on codec\r\n"
#if (XA_VIT_PRE_PROC == 1)
    " USAGE: record_dmic [language]\r\n"
    " For voice recognition say supported WakeWord and in 3s frame supported command.\r\n"
    " If selected model contains strings, then WakeWord and list of commands will be printed in console.\r\n"
#endif
    " NOTE: this command does not return to the shell\r\n",
    shellRecDMIC,
#if (XA_VIT_PRE_PROC == 1)
    1);
#else
    0);
#endif
#endif

SDK_ALIGN(static uint8_t s_shellHandleBuffer[SHELL_HANDLE_SIZE], 4);
static shell_handle_t s_shellHandle;

extern serial_handle_t g_serialHandle;
static handleShellMessageCallback_t *g_handleShellMessageCallback;
static void *g_handleShellMessageCallbackData;
/*${variable:end}*/

/*******************************************************************************
 * Code
 ******************************************************************************/

/*${function:start}*/
static void initMessage(srtm_message *msg)
{
    /* Common field for command */
    /* For single command, command list not used at the moment */
    msg->head.type = SRTM_MessageTypeRequest;

    msg->head.majorVersion = SRTM_VERSION_MAJOR;
    msg->head.minorVersion = SRTM_VERSION_MINOR;
}

static shell_status_t shellEcho(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    srtm_message msg = {0};
    initMessage(&msg);

    msg.head.category = SRTM_MessageCategory_GENERAL;
    msg.head.command  = SRTM_Command_ECHO;

    g_handleShellMessageCallback(&msg, g_handleShellMessageCallbackData);
    return kStatus_SHELL_Success;
}

#if XA_PCM_GAIN
static shell_status_t shellRecDMIC(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    srtm_message msg = {0};
    initMessage(&msg);

#if (defined(XA_VIT_PRE_PROC) && !defined(MIMXRT798S_cm33_core0_SERIES))
    BOARD_MuteRightChannel(true);
#elif (!defined(MIMXRT798S_cm33_core0_SERIES))
    BOARD_MuteRightChannel(BOARD_DMIC_NUM == 1);
#endif

    msg.head.category = SRTM_MessageCategory_AUDIO;
    msg.head.command  = SRTM_Command_REC_DMIC;
    /* Param 0 Number of Channels*/
    /* Param 1 Sampling Rate*/
    /* Param 2 PCM bit Width*/
    /* Param 3 return parameter, PCM buffer starting address*/
    /* Param 4 return parameter, buffer length*/
    /* Param 5 return parameter, number of buffers*/
    /* Param 6 return parameter, recording status: 0 un-initialized 1 recording 2 paused */
    /* Param 7 return parameter, error code*/

    msg.param[0] = BOARD_DMIC_NUM;
    msg.param[1] = 16000;
    msg.param[2] = BOARD_PCM_WIDTH;

#if (XA_VIT_PRE_PROC == 1)
#if (VIT_MODELS_ALL == 1)
    if (strcmp(argv[1], "en") == 0)
    {
        msg.param[3] = (uint32_t)&VIT_Model_en;
        msg.param[4] = (uint32_t)sizeof(VIT_Model_en);
        PRINTF("[CM33 CMD] Setting VIT language to en\r\n");
    }
    else if (strcmp(argv[1], "cn") == 0)
    {
        msg.param[3] = (uint32_t)&VIT_Model_cn;
        msg.param[4] = (uint32_t)sizeof(VIT_Model_cn);
        PRINTF("[CM33 CMD] Setting VIT language to cn\r\n");
    }
    else if (strcmp(argv[1], "de") == 0)
    {
        msg.param[3] = (uint32_t)&VIT_Model_de;
        msg.param[4] = (uint32_t)sizeof(VIT_Model_de);
        PRINTF("[CM33 CMD] Setting VIT language to de\r\n");
    }
    else if (strcmp(argv[1], "es") == 0)
    {
        msg.param[3] = (uint32_t)&VIT_Model_es;
        msg.param[4] = (uint32_t)sizeof(VIT_Model_es);
        PRINTF("[CM33 CMD] Setting VIT language to es\r\n");
    }
    else if (strcmp(argv[1], "fr") == 0)
    {
        msg.param[3] = (uint32_t)&VIT_Model_fr;
        msg.param[4] = (uint32_t)sizeof(VIT_Model_fr);
        PRINTF("[CM33 CMD] Setting VIT language to fr\r\n");
    }
    else if (strcmp(argv[1], "it") == 0)
    {
        msg.param[3] = (uint32_t)&VIT_Model_it;
        msg.param[4] = (uint32_t)sizeof(VIT_Model_it);
        PRINTF("[CM33 CMD] Setting VIT language to it\r\n");
    }
    else if (strcmp(argv[1], "ja") == 0)
    {
        msg.param[3] = (uint32_t)&VIT_Model_ja;
        msg.param[4] = (uint32_t)sizeof(VIT_Model_ja);
        PRINTF("[CM33 CMD] Setting VIT language to ja\r\n");
    }
    else if (strcmp(argv[1], "ko") == 0)
    {
        msg.param[3] = (uint32_t)&VIT_Model_ko;
        msg.param[4] = (uint32_t)sizeof(VIT_Model_ko);
        PRINTF("[CM33 CMD] Setting VIT language to ko\r\n");
    }
    else if (strcmp(argv[1], "tr") == 0)
    {
        msg.param[3] = (uint32_t)&VIT_Model_tr;
        msg.param[4] = (uint32_t)sizeof(VIT_Model_tr);
        PRINTF("[CM33 CMD] Setting VIT language to tr\r\n");
    }
    else if (strcmp(argv[1], "pt") == 0)
    {
        msg.param[3] = (uint32_t)&VIT_Model_pt;
        msg.param[4] = (uint32_t)sizeof(VIT_Model_pt);
        PRINTF("[CM33 CMD] Setting VIT language to pt\r\n");
    }
    else
    {
        PRINTF("[CM33 CMD] Wrong language selected.\r\n");
        return kStatus_SHELL_Success;
    }
#else
    if (strcmp(argv[1], "en") == 0)
    {
        msg.param[3] = (uint32_t)&VIT_Model_en;
        msg.param[4] = (uint32_t)sizeof(VIT_Model_en);
        PRINTF("[CM33 CMD] Setting VIT language to en\r\n");
    }
#endif
#endif

    g_handleShellMessageCallback(&msg, g_handleShellMessageCallbackData);
    return kStatus_SHELL_Success;
}
#endif

void shellCmd(handleShellMessageCallback_t *handleShellMessageCallback, void *arg)
{
    /* Init SHELL */
    s_shellHandle = &s_shellHandleBuffer[0];
    SHELL_Init(s_shellHandle, g_serialHandle, ">> ");

    /* Add new command to commands list */
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(version));

#if XA_PCM_GAIN
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(record_dmic));
#endif

    g_handleShellMessageCallback     = handleShellMessageCallback;
    g_handleShellMessageCallbackData = arg;

#if !(defined(SHELL_NON_BLOCKING_MODE) && (SHELL_NON_BLOCKING_MODE > 0U))
    SHELL_Task(s_shellHandle);
#endif
}

static void handleDSPMessageInner(app_handle_t *app, srtm_message *msg, bool *notify_shell)
{
    *notify_shell = true;

    char string_buff[SRTM_CMD_PARAMS_MAX];

    if (msg->head.type == SRTM_MessageTypeResponse)
    {
        PRINTF("[CM33 CMD] [APP_DSP_IPC_Task] response from DSP, cmd: %d, error: %d\r\n", msg->head.command,
               msg->error);
    }

    /* Processing returned data*/
    switch (msg->head.category)
    {
        case SRTM_MessageCategory_GENERAL:
            switch (msg->head.command)
            {
                /* echo returns version info of key components*/
                case SRTM_Command_ECHO:
                    PRINTF("[CM33 CMD] Component versions from DSP:\r\n");
                    PRINTF("[CM33 CMD] Audio Framework version %d.%d \r\n", msg->param[0] >> 16, msg->param[0] & 0xFF);
                    PRINTF("[CM33 CMD] Audio Framework API version %d.%d\r\n", msg->param[1] >> 16,
                           msg->param[1] & 0xFF);
                    PRINTF("[CM33 CMD] NatureDSP Lib version %d.%d\r\n", msg->param[2] >> 16, msg->param[2] & 0xFF);
                    PRINTF("[CM33 CMD] NatureDSP API version %d.%d\r\n", msg->param[3] >> 16, msg->param[3] & 0xFF);
                    break;

                case SRTM_Command_SYST:
                    break;
                default:
                    PRINTF("[CM33 CMD] Incoming unknown message command %d from category %d \r\n", msg->head.command,
                           msg->head.category);
            }
            break;

        case SRTM_MessageCategory_AUDIO:
            switch (msg->head.command)
            {
                case SRTM_Command_REC_DMIC:
                    if (msg->error != SRTM_Status_Success)
                    {
                        PRINTF("[CM33 CMD] DSP Recording start failed! return error = %d\r\n", msg->error);
                        break;
                    }
                    PRINTF("[CM33 CMD] DSP DMIC Recording started\r\n");
                    vTaskSuspend(app->shell_task_handle);
                    break;

                case SRTM_Command_REC_I2S:
                    if (msg->error != SRTM_Status_Success)
                    {
                        PRINTF("[CM33 CMD] DSP Recording start failed! return error = %d\r\n", msg->error);
                    }
                    break;

                case SRTM_Command_VIT:
                    PRINTF("[CM33 CMD] DSP DMIC Recording started\r\n");
                    PRINTF("[CM33 CMD] To see VIT functionality say wakeword and command\r\n");
                    vTaskSuspend(app->shell_task_handle);
                    break;

                case SRTM_Print_String:
                    for (int i = 0; i < SRTM_CMD_PARAMS_MAX; i++)
                    {
                        string_buff[i] = (char)msg->param[i];
                    }
                    PRINTF("[CM33 CMD] %s", string_buff);
                    break;

                default:
                    PRINTF("[CM33 CMD] Incoming unknown message category %d \r\n", msg->head.category);
                    break;
            }
            break;
    }
}

void handleDSPMessage(app_handle_t *app, srtm_message *msg)
{
    bool notify_shell = false;

    handleDSPMessageInner(app, msg, &notify_shell);

    if (notify_shell)
    {
        /* Signal to shell that response has been processed. */
        if (app->shell_task_handle != NULL)
        {
            xTaskNotifyGive(app->shell_task_handle);
        }
    }
}
/*${function:end}*/
