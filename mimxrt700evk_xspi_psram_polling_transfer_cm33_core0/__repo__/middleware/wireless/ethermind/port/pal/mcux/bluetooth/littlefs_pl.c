
/*
 * Copyright 2021 - 2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#if ((defined(CONFIG_BT_SETTINGS)) && (CONFIG_BT_SETTINGS))

#include "fsl_common.h"

#ifdef EDGEFAST_BT_LITTLEFS_MFLASH
#include "mflash_common.h"
#include "mflash_drv.h"
#else
#include "fsl_adapter_flash.h"
#endif

#include "fsl_os_abstraction.h"

#include "littlefs_pl.h"

#ifdef LITTLEFS_PL_DEBUG
#include "fsl_debug_console.h"
#endif /* LITTLEFS_PL_DEBUG */

/* Maximum block read size definition */
#ifndef LITTLEFS_READ_SIZE
#define LITTLEFS_READ_SIZE 16
#endif
/* Maximum block program size definition */
#ifndef LITTLEFS_PROG_SIZE
#ifdef EDGEFAST_BT_LITTLEFS_MFLASH
#define LITTLEFS_PROG_SIZE MFLASH_PAGE_SIZE
#else
#define LITTLEFS_PROG_SIZE 256
#endif
#endif
/* Erasable block size definition */
#ifndef LITTLEFS_BLOCK_SIZE
#ifdef EDGEFAST_BT_LITTLEFS_MFLASH
#define LITTLEFS_BLOCK_SIZE MFLASH_SECTOR_SIZE
#else
#define LITTLEFS_BLOCK_SIZE 4096
#endif
#endif
/* Block count */
#ifndef LITTLEFS_BLOCK_COUNT
#define LITTLEFS_BLOCK_COUNT 1024
#endif

/* Minimum block cache size definition */
#ifndef LITTLEFS_CACHE_SIZE
#define LITTLEFS_CACHE_SIZE 256
#endif
/* Minimum lookahead buffer size definition */
#ifndef LITTLEFS_LOOKAHEAD_SIZE
#define LITTLEFS_LOOKAHEAD_SIZE 16
#endif

#if (!(defined(__CC_ARM) || defined(__ARMCC_VERSION)))
/*
 * Name: EDGEFAST_BT_LITTLEFS_STORAGE_START_ADDRESS
 * Description: EDGEFAST_BT_LITTLEFS_STORAGE_START_ADDRESS from linker command file is used by this code
 *              as Raw Sector Start Address.
 */
extern uint32_t EDGEFAST_BT_LITTLEFS_STORAGE_START_ADDRESS[];

/*
 * Name: EDGEFAST_BT_LITTLEFS_STORAGE_SECTOR_SIZE
 * Description: external symbol from linker command file, it represents the size
 *              of a FLASH sector
 */
extern uint32_t EDGEFAST_BT_LITTLEFS_STORAGE_SECTOR_SIZE[];

/*
 * Name:  EDGEFAST_BT_LITTLEFS_STORAGE_MAX_SECTORS
 * Description: external symbol from linker command file, it represents the sectors
 *              count used by the ENVM storage system; it has to be a multiple of 2
 */
extern uint32_t EDGEFAST_BT_LITTLEFS_STORAGE_MAX_SECTORS[];
#else

extern uint32_t Image$$EDGEFAST_BT_LittleFS_region$$ZI$$Base[];
extern uint32_t Image$$EDGEFAST_BT_LittleFS_region$$ZI$$Limit[];
extern uint32_t Image$$EDGEFAST_BT_LittleFS_region$$Length;

#define EDGEFAST_BT_LITTLEFS_STORAGE_START_ADDRESS (Image$$EDGEFAST_BT_LittleFS_region$$ZI$$Base)
#define EDGEFAST_BT_LITTLEFS_STORAGE_END_ADDRESS (Image$$EDGEFAST_BT_LittleFS_region$$ZI$$Limit)
#define NVM_LENGTH ((uint32_t)((uint8_t*)EDGEFAST_BT_LITTLEFS_STORAGE_END_ADDRESS)-(uint32_t)((uint8_t*)EDGEFAST_BT_LITTLEFS_STORAGE_START_ADDRESS))
#define EDGEFAST_BT_LITTLEFS_STORAGE_SECTOR_SIZE FSL_FEATURE_FLASH_PAGE_SIZE_BYTES
#define EDGEFAST_BT_LITTLEFS_STORAGE_MAX_SECTORS (NVM_LENGTH/EDGEFAST_BT_LITTLEFS_STORAGE_SECTOR_SIZE)
#endif /* __CC_ARM */

typedef struct
{
    uint32_t start_addr;
} lfs_mflash_ctx_t;

static int lfs_mflash_read(const struct lfs_config *lfsc, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
static int lfs_mflash_prog(
    const struct lfs_config *lfsc, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
static int lfs_mflash_erase(const struct lfs_config *lfsc, lfs_block_t block);
static int lfs_mflash_sync(const struct lfs_config *lfsc);

static lfs_mflash_ctx_t LittleFS_ctx;

#ifdef LFS_THREADSAFE
static int lfs_mflash_lock(const struct lfs_config *lfsc);
static int lfs_mflash_unlock(const struct lfs_config *lfsc);
#endif
static struct lfs_config LittleFS_config = {
  .context = (void*)&LittleFS_ctx,
  .read = lfs_mflash_read,
  .prog = lfs_mflash_prog,
  .erase = lfs_mflash_erase,
  .sync = lfs_mflash_sync,
#ifdef LFS_THREADSAFE
  .lock = lfs_mflash_lock,
  .unlock = lfs_mflash_unlock,
#endif
  .read_size = LITTLEFS_READ_SIZE,
  .prog_size = LITTLEFS_PROG_SIZE,
  .block_size = LITTLEFS_BLOCK_SIZE,
  .block_count = LITTLEFS_BLOCK_COUNT,
  .block_cycles = 100,
  .cache_size = LITTLEFS_CACHE_SIZE,
  .lookahead_size = LITTLEFS_LOOKAHEAD_SIZE
};

static lfs_t lfs_pl;
static OSA_MUTEX_HANDLE_DEFINE(s_flashOpsLock);

static int lfs_mflash_read(const struct lfs_config *lfsc, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    uint32_t flash_addr;

    assert(lfsc);
#ifdef EDGEFAST_BT_LITTLEFS_MFLASH
    flash_addr = LittleFS_ctx.start_addr + block * lfsc->block_size + off;
#else
    flash_addr = ((uint32_t)EDGEFAST_BT_LITTLEFS_STORAGE_START_ADDRESS) + block * lfsc->block_size + off;
#endif
#ifndef LFS_THREADSAFE
	(void)OSA_MutexLock((osa_mutex_handle_t)s_flashOpsLock, osaWaitForever_c);
#endif

#ifdef EDGEFAST_BT_LITTLEFS_MFLASH
    if (mflash_drv_read(flash_addr, buffer, size) != kStatus_Success)
#else
    if (HAL_FlashRead(flash_addr, size, buffer) != kStatus_HAL_Flash_Success)
#endif
	{
#ifndef LFS_THREADSAFE
        (void)OSA_MutexUnlock((osa_mutex_handle_t)s_flashOpsLock);
#endif
        return LFS_ERR_IO;
    }
#ifndef LFS_THREADSAFE
    (void)OSA_MutexUnlock((osa_mutex_handle_t)s_flashOpsLock);
#endif

    return LFS_ERR_OK;
}

static int lfs_mflash_prog(
    const struct lfs_config *lfsc, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    status_t status = kStatus_Fail;
    uint32_t flash_addr;
#ifdef LITTLEFS_PL_DEBUG
    static uint64_t totalTime = 0;
#endif /* LITTLEFS_PL_DEBUG */

    assert(lfsc);

#ifdef EDGEFAST_BT_LITTLEFS_MFLASH
    flash_addr = LittleFS_ctx.start_addr + block * lfsc->block_size + off;
#else
    flash_addr = ((uint32_t)EDGEFAST_BT_LITTLEFS_STORAGE_START_ADDRESS) + block * lfsc->block_size + off;
#endif
#ifndef LFS_THREADSAFE
    (void)OSA_MutexLock((osa_mutex_handle_t)s_flashOpsLock, osaWaitForever_c);
#endif
    for (uint32_t page_ofs = 0; page_ofs < size; page_ofs += LITTLEFS_PROG_SIZE)
    {
#ifdef LITTLEFS_PL_DEBUG
        DWT->CYCCNT = 0;
        DWT->CTRL |= (1 << DWT_CTRL_CYCCNTENA_Pos);
#endif /* LITTLEFS_PL_DEBUG */

#ifdef EDGEFAST_BT_LITTLEFS_MFLASH
        status = mflash_drv_page_program(flash_addr + page_ofs, (void *)((uintptr_t)buffer + page_ofs));
#else
        status = HAL_FlashProgram(flash_addr + page_ofs, LITTLEFS_PROG_SIZE, (void *)((uintptr_t)buffer + page_ofs));
#endif

#ifdef LITTLEFS_PL_DEBUG
        totalTime = DWT->CYCCNT;
        PRINTF("pt %dms\r\n", (uint32_t)((totalTime) / (configCPU_CLOCK_HZ / 1000)));
#endif /* LITTLEFS_PL_DEBUG */
        if (status != kStatus_Success)
        {
            break;
        }
    }
#ifndef LFS_THREADSAFE
    (void)OSA_MutexUnlock((osa_mutex_handle_t)s_flashOpsLock);
#endif
    if (status != kStatus_Success)
    {
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

static int lfs_mflash_erase(const struct lfs_config *lfsc, lfs_block_t block)
{
    status_t status = kStatus_Fail;
    uint32_t flash_addr;
#ifdef LITTLEFS_PL_DEBUG
    uint64_t totalTime = 0;
#endif /* LITTLEFS_PL_DEBUG */

    assert(lfsc);

#ifdef EDGEFAST_BT_LITTLEFS_MFLASH
    flash_addr = LittleFS_ctx.start_addr + block * lfsc->block_size;
#else
    flash_addr = ((uint32_t)EDGEFAST_BT_LITTLEFS_STORAGE_START_ADDRESS) + block * lfsc->block_size;
#endif
#ifndef LFS_THREADSAFE
    (void)OSA_MutexLock((osa_mutex_handle_t)s_flashOpsLock, osaWaitForever_c);
#endif
    for (uint32_t sector_ofs = 0; sector_ofs < lfsc->block_size; sector_ofs += LITTLEFS_BLOCK_SIZE)
    {
#ifdef LITTLEFS_PL_DEBUG
        DWT->CYCCNT = 0;
        DWT->CTRL |= (1 << DWT_CTRL_CYCCNTENA_Pos);
#endif /* LITTLEFS_PL_DEBUG */

#ifdef EDGEFAST_BT_LITTLEFS_MFLASH
        status = mflash_drv_sector_erase(flash_addr + sector_ofs);
#else
        status = HAL_FlashEraseSector(flash_addr + sector_ofs, LITTLEFS_BLOCK_SIZE);
#endif
#ifdef LITTLEFS_PL_DEBUG
        totalTime = DWT->CYCCNT;
        PRINTF("et %dms\r\n", (uint32_t)((totalTime) / (configCPU_CLOCK_HZ / 1000)));
#endif /* LITTLEFS_PL_DEBUG */

        if (status != kStatus_Success)
        {
            break;
        }
    }
#ifndef LFS_THREADSAFE
    (void)OSA_MutexUnlock((osa_mutex_handle_t)s_flashOpsLock);
#endif
    if (status != kStatus_Success)
    {
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

static int lfs_mflash_sync(const struct lfs_config *lfsc)
{
    return LFS_ERR_OK;
}

#ifdef LFS_THREADSAFE
static int lfs_mflash_lock(const struct lfs_config *lfsc)
{
    assert(lfsc);
    (void)OSA_MutexLock((osa_mutex_handle_t)s_flashOpsLock, osaWaitForever_c);
    return LFS_ERR_OK;
}
static int lfs_mflash_unlock(const struct lfs_config *lfsc)
{
    assert(lfsc);
    (void)OSA_MutexUnlock((osa_mutex_handle_t)s_flashOpsLock); 
    return LFS_ERR_OK;
}
#endif

lfs_t * lfs_pl_init(void)
{
    static uint8_t initialized = 0;
    int error = 0;
    osa_status_t ret;

    if (0 == initialized)
    {
        LittleFS_config.block_count = (uint32_t)EDGEFAST_BT_LITTLEFS_STORAGE_MAX_SECTORS;

#ifdef EDGEFAST_BT_LITTLEFS_MFLASH
        /* mflash driver requires address offset based on flash base address
         * but LITTLEFS_STORAGE_START_ADDRESS is an absolute address exported by linker script
         * so we need to convert LITTLEFS_STORAGE_START_ADDRESS to an offset */
        LittleFS_ctx.start_addr  = ((uint32_t)EDGEFAST_BT_LITTLEFS_STORAGE_START_ADDRESS) & ~(MFLASH_BASE_ADDRESS);
#else
        LittleFS_ctx.start_addr = ((uint32_t)EDGEFAST_BT_LITTLEFS_STORAGE_START_ADDRESS);
#endif

#ifdef LITTLEFS_PL_DEBUG
        CoreDebug->DEMCR |= (1 << CoreDebug_DEMCR_TRCENA_Pos);
#endif /* LITTLEFS_PL_DEBUG */

        ret = OSA_MutexCreate((osa_mutex_handle_t)s_flashOpsLock);
        assert(KOSA_StatusSuccess == ret);
        if (KOSA_StatusSuccess != ret)
        {
            error = -1;
        }
        else
        {
#ifdef EDGEFAST_BT_LITTLEFS_MFLASH
            /* Init Flash */
            mflash_drv_init();
#else
            HAL_FlashInit();
#endif

            error = lfs_mount(&lfs_pl, &LittleFS_config);

            if (LFS_ERR_CORRUPT == error)
            {
                error = lfs_format(&lfs_pl, &LittleFS_config);
                if (0 <= error)
                {
                    error = lfs_mount(&lfs_pl, &LittleFS_config);
                    assert(0 <= error);
                }
            }
            initialized = 1;
        }
    }

    if (0 == error)
    {
        return &lfs_pl;
    }
    else
    {
        return NULL;
    }
}
#endif /* CONFIG_BT_SETTINGS */
