/*
 * Copyright (c) 2015, 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rpmsg_lite.h"
#include "board.h"
#include "erpc_server_setup.h"
#include "c_erpc_matrix_multiply_server.h"
#include "erpc_matrix_multiply_common.h"
#include "erpc_error_handler.h"
#include "app.h"
#include "mcmgr.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define APP_ERPC_READY_EVENT_DATA (1U)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief erpcMatrixMultiply function implementation.
 *
 * This is the implementation of the erpcMatrixMultiply function called by the primary core.
 *
 * @param matrix1 First matrix
 * @param matrix2 Second matrix
 * @param result_matrix Result matrix
 */
void erpcMatrixMultiply(Matrix matrix1, Matrix matrix2, Matrix result_matrix)
{
    int32_t i, j, k;

    /* Clear the result matrix */
    for (i = 0; i < matrix_size; ++i)
    {
        for (j = 0; j < matrix_size; ++j)
        {
            result_matrix[i][j] = 0;
        }
    }

    /* Multiply two matrices */
    for (i = 0; i < matrix_size; ++i)
    {
        for (j = 0; j < matrix_size; ++j)
        {
            for (k = 0; k < matrix_size; ++k)
            {
                result_matrix[i][j] += matrix1[i][k] * matrix2[k][j];
            }
        }
    }
}

static void SignalReady(void)
{
    /* Signal the other core we are ready by triggering the event and passing the APP_ERPC_READY_EVENT_DATA */
    (void)MCMGR_TriggerEvent(kMCMGR_RemoteApplicationEvent, APP_ERPC_READY_EVENT_DATA);
}

/*!
 * @brief Main function
 */
int main(void)
{
    uint32_t startupData;
    mcmgr_status_t mcmgrStatus;

    BOARD_InitHardware();

    /* Initialize MCMGR before calling its API */
    (void)MCMGR_Init();

    /* Get the startup data */
    do
    {
        mcmgrStatus = MCMGR_GetStartupData(&startupData);
    } while (mcmgrStatus != kStatus_MCMGR_Success);

    /* RPMsg-Lite transport layer initialization */
    erpc_transport_t transport;
    erpc_server_t server;

    transport = erpc_transport_rpmsg_lite_remote_init(101U, 100U, (void *)(char *)(platform_patova(startupData)),
                                                      ERPC_TRANSPORT_RPMSG_LITE_LINK_ID, SignalReady, NULL);

    /* MessageBufferFactory initialization */
    erpc_mbf_t message_buffer_factory;
    message_buffer_factory = erpc_mbf_rpmsg_init(transport);

    /* eRPC server side initialization */
    server = erpc_server_init(transport, message_buffer_factory);

    /* adding the service to the server */
    erpc_service_t service = create_MatrixMultiplyService_service();
    erpc_add_service_to_server(server, service);

    for (;;)
    {
        /* process message */
        erpc_status_t status = erpc_server_poll(server);

        /* handle error status */
        if (status != (erpc_status_t)kErpcStatus_Success)
        {
            /* print error description */
            erpc_error_handler(status, 0);

            /* removing the service from the server */
            erpc_remove_service_from_server(server, service);
            destroy_MatrixMultiplyService_service(service);

            /* stop erpc server */
            erpc_server_stop(server);

            /* print error description */
            erpc_server_deinit(server);

            /* exit program loop */
            break;
        }

        /* do other tasks */
        int32_t i;
        for (i = 0; i < 10000; i++)
        {
        }
    }
    for (;;)
    {
    }
}
