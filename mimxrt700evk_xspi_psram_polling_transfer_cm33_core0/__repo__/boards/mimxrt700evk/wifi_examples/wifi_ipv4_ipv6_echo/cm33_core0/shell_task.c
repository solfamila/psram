/*
 * Copyright 2022-2023, 2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "shell_task.h"
#include "shell_task_mode.h"

#include "lwip/sys.h"

#include "fsl_component_serial_manager.h"

#include "socket_task.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static shell_status_t echo_tcp_client(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t echo_tcp_server(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t echo_udp(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t end(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t print_ip_cfg(shell_handle_t shellHandle, int32_t argc, char **argv);

/*******************************************************************************
 * Variables
 ******************************************************************************/

SDK_ALIGN(static uint8_t s_shellHandleBuffer[SHELL_HANDLE_SIZE], 4);
static shell_handle_t s_shellHandle;

extern serial_handle_t g_serialHandle;

static int s_is_in_default_mode = 1;

SHELL_COMMAND_DEFINE(echo_tcp_client,
                     "\r\n\"echo_tcp_client ip_addr port\":\r\n"
                     "   Connects to specified server and sends back every received data.\r\n"
                     " Usage:\r\n"
                     "   ip_addr:     IPv6 or IPv4 server address\r\n"
                     "   port:        TCP port number\r\n",
                     echo_tcp_client,
                     2);

SHELL_COMMAND_DEFINE(echo_tcp_server,
                     "\r\n\"echo_tcp_server port\":\r\n"
                     "   Listens for one incoming connection and sends back every received data.\r\n"
                     " Usage:\r\n"
                     "   port:        TCP port number\r\n",
                     echo_tcp_server,
                     1);

SHELL_COMMAND_DEFINE(echo_udp,
                     "\r\n\"echo_udp port\":\r\n"
                     "   Waits for datagrams and sends them back.\r\n"
                     " Usage:\r\n"
                     "   port:        UDP port number\r\n",
                     echo_udp,
                     1);

SHELL_COMMAND_DEFINE(end, "\r\n\"end\": Ends echo_* command.\r\n", end, 0);
SHELL_COMMAND_DEFINE(print_ip_cfg, "\r\n\"print_ip_cfg\": Prints IP configuration.\r\n", print_ip_cfg, 0);

/*******************************************************************************
 * Code
 ******************************************************************************/

static void call_socket_task_init(int is_tcp, const char *ip_str, const char *port_str)
{
    if (s_is_in_default_mode)
    {
        int ret;

        shell_task_set_mode(SHELL_MODE_NO_PROMPT);

        ret = socket_task_init(is_tcp, ip_str, port_str);
        if (ret < 0)
        {
            shell_task_set_mode(SHELL_MODE_DEFAULT);
            SHELL_Printf(s_shellHandle, "\r\n");
        }
    }
    else
    {
        SHELL_Printf(s_shellHandle, "Busy.\r\n");
    }
}

static shell_status_t echo_tcp_client(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    (void)shellHandle;
    (void)argc;

    call_socket_task_init(1, argv[1], argv[2]);

    return kStatus_SHELL_Success;
}

static shell_status_t echo_tcp_server(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    (void)shellHandle;
    (void)argc;

    call_socket_task_init(1, NULL, argv[1]);

    return kStatus_SHELL_Success;
}

static shell_status_t echo_udp(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    (void)shellHandle;
    (void)argc;

    call_socket_task_init(0, NULL, argv[1]);

    return kStatus_SHELL_Success;
}

static shell_status_t end(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    (void)shellHandle;
    (void)argc;

    socket_task_terminate();

    return kStatus_SHELL_Success;
}

static shell_status_t print_ip_cfg(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    (void)shellHandle;
    (void)argc;

    socket_task_print_ips();

    // SHELL_Printf(s_shellHandle, "\r\nheap=%U\r\n", xPortGetMinimumEverFreeHeapSize());

    return kStatus_SHELL_Success;
}

void shell_task_init(shell_command_t **additional_commands)
{
#if !(defined(SHELL_NON_BLOCKING_MODE) && (SHELL_NON_BLOCKING_MODE > 0U))
#error "Blocking shell is not supported, it does not let run IDLE task and the stacks of self-deleted tasks are not deallocated."
#endif

    /* Init SHELL */
    s_shellHandle = &s_shellHandleBuffer[0];
    
    /* Shell task is created in SHELL_Init if shell is non-blocking */
    SHELL_Init(s_shellHandle, g_serialHandle, SHELL_MODE_DEFAULT);

    socket_task_set_shell(s_shellHandle);

    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(echo_tcp_client));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(echo_tcp_server));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(echo_udp));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(end));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(print_ip_cfg));

    if (additional_commands != NULL)
    {
        for (; *additional_commands; (void)*additional_commands++)
            SHELL_RegisterCommand(s_shellHandle, *additional_commands);
    }
}

void shell_task_set_mode(const char *mode)
{
    static char prompt[32];

    strncpy(prompt, mode, sizeof(prompt) - 1);
    prompt[sizeof(prompt) - 1] = '\0';

    s_is_in_default_mode = (strcmp(mode, SHELL_MODE_DEFAULT) == 0);
    SHELL_ChangePrompt(s_shellHandle, prompt);
}
