/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "diskio_impl.h"
#include "ffconf.h"
#include "ff.h"
#include "sdmmc_types.h"
#include "esp_log.h"
#include "esp_compiler.h"

static sdmmc_card_t* s_cards[FF_VOLUMES] = { NULL };

static const char* TAG = "diskio_sdmmc";

//Check if SD/MMC card is present
static DSTATUS ff_sdmmc_card_available(BYTE pdrv)
{
    // sdmmc_card_t* card = s_cards[pdrv];
    // assert(card);
    // esp_err_t err = sdmmc_get_status(card);
    // if (unlikely(err != ESP_OK)) {
    //     ESP_LOGE(TAG, "Check status failed (0x%x)", err);
    //     return STA_NOINIT;
    // }
    return 0;
}

/**
*   ff_sdmmc_status() and ff_sdmmc_initialize() return STA_NOINIT when sdmmc_get_status()
*   fails. This error value is checked throughout the FATFS code.
*   Both functions return 0 on success.
*/
DSTATUS ff_sdmmc_initialize (BYTE pdrv)
{
    return ff_sdmmc_card_available(pdrv);
}

DSTATUS ff_sdmmc_status (BYTE pdrv)
{
    return ff_sdmmc_card_available(pdrv);
}

DRESULT ff_sdmmc_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count)
{
    // sdmmc_card_t* card = s_cards[pdrv];
    // assert(card);
    // esp_err_t err = sdmmc_read_sectors(card, buff, sector, count);
    // if (unlikely(err != ESP_OK)) {
    //     ESP_LOGE(TAG, "sdmmc_read_blocks failed (%d)", err);
    //     return RES_ERROR;
    // }
    return RES_OK;
}

DRESULT ff_sdmmc_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count)
{
    // sdmmc_card_t* card = s_cards[pdrv];
    // assert(card);
    // esp_err_t err = sdmmc_write_sectors(card, buff, sector, count);
    // if (unlikely(err != ESP_OK)) {
    //     ESP_LOGE(TAG, "sdmmc_write_blocks failed (%d)", err);
    //     return RES_ERROR;
    // }
    return RES_OK;
}

DRESULT ff_sdmmc_ioctl (BYTE pdrv, BYTE cmd, void* buff)
{
    sdmmc_card_t* card = s_cards[pdrv];
    assert(card);
    switch(cmd) {
        case CTRL_SYNC:
            return RES_OK;
        case GET_SECTOR_COUNT:
            *((DWORD*) buff) = card->csd.capacity;
            return RES_OK;
        case GET_SECTOR_SIZE:
            *((WORD*) buff) = card->csd.sector_size;
            return RES_OK;
        case GET_BLOCK_SIZE:
            return RES_ERROR;
    }
    return RES_ERROR;
}

void ff_diskio_register_sdmmc(BYTE pdrv, sdmmc_card_t* card)
{
    static const ff_diskio_impl_t sdmmc_impl = {
        .init = &ff_sdmmc_initialize,
        .status = &ff_sdmmc_status,
        .read = &ff_sdmmc_read,
        .write = &ff_sdmmc_write,
        .ioctl = &ff_sdmmc_ioctl
    };
    s_cards[pdrv] = card;
    ff_diskio_register(pdrv, &sdmmc_impl);
}

BYTE ff_diskio_get_pdrv_card(const sdmmc_card_t* card)
{
    for (int i = 0; i < FF_VOLUMES; i++) {
        if (card == s_cards[i]) {
            return i;
        }
    }
    return 0xff;
}
