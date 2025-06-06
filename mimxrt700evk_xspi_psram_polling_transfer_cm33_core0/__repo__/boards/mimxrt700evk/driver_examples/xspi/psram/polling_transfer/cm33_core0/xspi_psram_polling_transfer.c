/*
 * Copyright 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_xspi.h"
#include "fsl_debug_console.h"
#include "app.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
static uint8_t s_hyper_ram_write_buffer[256];
/* psram enable x16 mode address will offset 8 byte*/
static uint8_t s_hyper_ram_read_buffer[256];
extern void xspi_hyper_ram_init(XSPI_Type *base);
extern void xspi_hyper_ram_ahbcommand_write_data(XSPI_Type *base, uint32_t address, uint32_t *buffer, uint32_t length);
extern void xspi_hyper_ram_ahbcommand_read_data(XSPI_Type *base, uint32_t address, uint32_t *buffer, uint32_t length);
extern status_t xspi_hyper_ram_ipcommand_write_data(XSPI_Type *base,
                                                    uint32_t address,
                                                    uint32_t *buffer,
                                                    uint32_t length);
extern status_t xspi_hyper_ram_ipcommand_read_data(XSPI_Type *base,
                                                   uint32_t address,
                                                   uint32_t *buffer,
                                                   uint32_t length);
/*******************************************************************************
 * Code
 ******************************************************************************/

int main(void)
{
    uint32_t i  = 0;
    status_t st = kStatus_Success;

    BOARD_InitHardware();
    /* XSPI init */
    xspi_hyper_ram_init(EXAMPLE_XSPI);

    PRINTF("XSPI example started!\r\n");

    for (i = 0; i < sizeof(s_hyper_ram_write_buffer); i++)
    {
        s_hyper_ram_write_buffer[i] = (i + 0xFFU);
    }

    memset(s_hyper_ram_read_buffer, 0, sizeof(s_hyper_ram_read_buffer));

    for (i = 0; i < DRAM_SIZE; i += 1024)
    {
        xspi_hyper_ram_ahbcommand_write_data(EXAMPLE_XSPI, i, (uint32_t *)s_hyper_ram_write_buffer,
                                             sizeof(s_hyper_ram_write_buffer));
        xspi_hyper_ram_ahbcommand_read_data(EXAMPLE_XSPI, i, (uint32_t *)s_hyper_ram_read_buffer,
                                            sizeof(s_hyper_ram_read_buffer));
        if (memcmp(s_hyper_ram_read_buffer, s_hyper_ram_write_buffer, sizeof(s_hyper_ram_write_buffer)) != 0)
        {
            PRINTF("AHB Command Read/Write data Failure at 0x%x - 0x%x!\r\n", i, i + 1023);
            return -1;
        }
    }

    for (i = 0; i < sizeof(s_hyper_ram_write_buffer); i++)
    {
        s_hyper_ram_write_buffer[i] = i;
    }
    memset(s_hyper_ram_read_buffer, 0, sizeof(s_hyper_ram_read_buffer));

    for (i = 1; i < DRAM_SIZE - 1024; i += 1024)
    {
        xspi_hyper_ram_ahbcommand_write_data(EXAMPLE_XSPI, i, (uint32_t *)s_hyper_ram_write_buffer,
                                             sizeof(s_hyper_ram_write_buffer));
        xspi_hyper_ram_ahbcommand_read_data(EXAMPLE_XSPI, i, (uint32_t *)s_hyper_ram_read_buffer,
                                            sizeof(s_hyper_ram_read_buffer));

        if (memcmp(s_hyper_ram_read_buffer, s_hyper_ram_write_buffer, sizeof(s_hyper_ram_write_buffer)) != 0)
        {
            PRINTF("AHB Command Read/Write data Failure at 0x%x - 0x%x!\r\n", i, i + 1023);
            return -1;
        }
    }

    for (i = 0; i < sizeof(s_hyper_ram_write_buffer); i++)
    {
        s_hyper_ram_write_buffer[i] = (i + 0xFFU);
    }
    memset(s_hyper_ram_read_buffer, 0, sizeof(s_hyper_ram_read_buffer));

    for (i = 2; i < DRAM_SIZE - 1024; i += 1024)
    {
        xspi_hyper_ram_ahbcommand_write_data(EXAMPLE_XSPI, i, (uint32_t *)s_hyper_ram_write_buffer,
                                             sizeof(s_hyper_ram_write_buffer));
        xspi_hyper_ram_ahbcommand_read_data(EXAMPLE_XSPI, i, (uint32_t *)s_hyper_ram_read_buffer,
                                            sizeof(s_hyper_ram_read_buffer));

        if (memcmp(s_hyper_ram_read_buffer, s_hyper_ram_write_buffer, sizeof(s_hyper_ram_write_buffer)) != 0)
        {
            PRINTF("AHB Command Read/Write data Failure at 0x%x - 0x%x!\r\n", i, i + 1023);
            return -1;
        }
    }

    for (i = 0; i < sizeof(s_hyper_ram_write_buffer); i++)
    {
        s_hyper_ram_write_buffer[i] = i;
    }
    memset(s_hyper_ram_read_buffer, 0, sizeof(s_hyper_ram_read_buffer));

    for (i = 3; i < DRAM_SIZE - 1024; i += 1024)
    {
        xspi_hyper_ram_ahbcommand_write_data(EXAMPLE_XSPI, i, (uint32_t *)s_hyper_ram_write_buffer,
                                             sizeof(s_hyper_ram_write_buffer));
        xspi_hyper_ram_ahbcommand_read_data(EXAMPLE_XSPI, i, (uint32_t *)s_hyper_ram_read_buffer,
                                            sizeof(s_hyper_ram_read_buffer));

        if (memcmp(s_hyper_ram_read_buffer, s_hyper_ram_write_buffer, sizeof(s_hyper_ram_write_buffer)) != 0)
        {
            PRINTF("AHB Command Read/Write data Failure at 0x%x - 0x%x!\r\n", i, i + 1023);
            return -1;
        }
    }

    PRINTF("AHB Command Read/Write data successfully at all address range !\r\n");

    memset(s_hyper_ram_read_buffer, 0, sizeof(s_hyper_ram_read_buffer));
    for (i = 0; i < sizeof(s_hyper_ram_write_buffer); i++)
    {
        s_hyper_ram_write_buffer[i] = i;
    }

    /* IP command write/read, should notice that the start address should be even address and the write address/size
     * should be 1024 aligned.*/
    for (i = 0; i < DRAM_SIZE; i += 256)
    {
        st = xspi_hyper_ram_ipcommand_write_data(EXAMPLE_XSPI, i, (uint32_t *)s_hyper_ram_write_buffer,
                                                 sizeof(s_hyper_ram_write_buffer));

        if (st != kStatus_Success)
        {
            st = kStatus_Fail;
            PRINTF("IP Command Write data Failure at 0x%x!\r\n", i);
        }

        st = xspi_hyper_ram_ipcommand_read_data(EXAMPLE_XSPI, i, (uint32_t *)s_hyper_ram_read_buffer,
                                                sizeof(s_hyper_ram_read_buffer));
        if (st != kStatus_Success)
        {
            st = kStatus_Fail;
            PRINTF("IP Command Read data Failure at 0x%x!\r\n", i);
        }

        if (memcmp(s_hyper_ram_read_buffer, s_hyper_ram_write_buffer, sizeof(s_hyper_ram_write_buffer)) != 0)
        {
            PRINTF("IP Command Read/Write data Failure at 0x%x - 0x%x!\r\n", i, i + 255);
            return -1;
        }
    }

    PRINTF("IP Command Read/Write data successfully at all address range !\r\n");

    while (1)
    {
    }
}
