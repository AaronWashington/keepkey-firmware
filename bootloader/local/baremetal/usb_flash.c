/* START KEEPKEY LICENSE */
/*
 * This file is part of the KeepKey project.
 *
 * Copyright (C) 2015 KeepKey LLC
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/* END KEEPKEY LICENSE */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <libopencm3/stm32/flash.h>

#include <sha2.h>
#include <interface.h>
#include <keepkey_board.h>
#include <memory.h>
#include <msg_dispatch.h>
#include <keepkey_usart.h>
#include <confirm_sm.h>
#include <usb_driver.h>
#include <keepkey_flash.h>

#include "signatures.h"
#include "usb_flash.h"

#define RESP_INIT(TYPE) TYPE resp; memset(&resp, 0, sizeof(TYPE));

/*** Definition ***/
static FirmwareUploadState upload_state = UPLOAD_NOT_STARTED;
static ConfigFlash storage_shadow;
static uint8_t firmware_hash[SHA256_DIGEST_LENGTH];
extern bool reset_msg_stack;

/*** Structure to map incoming messages to handler functions. ***/
static const MessagesMap_t MessagesMap[] =
{
    // in messages
    MSG_IN(MessageType_MessageType_Initialize,              Initialize_fields,          (message_handler_t)(handler_initialize))
    MSG_IN(MessageType_MessageType_Ping,                    Ping_fields,                (message_handler_t)(handler_ping))
    MSG_IN(MessageType_MessageType_FirmwareErase,           FirmwareErase_fields,       (message_handler_t)(handler_erase))
    MSG_IN(MessageType_MessageType_ButtonAck,               ButtonAck_fields,           NO_PROCESS_FUNC)
    RAW_IN(MessageType_MessageType_FirmwareUpload,          FirmwareUpload_fields,      (message_handler_t)(raw_handler_upload))
    MSG_OUT(MessageType_MessageType_Features,               Features_fields,            NO_PROCESS_FUNC)
    MSG_OUT(MessageType_MessageType_Success,                Success_fields,             NO_PROCESS_FUNC)
    MSG_OUT(MessageType_MessageType_Failure,                Failure_fields,             NO_PROCESS_FUNC)
    MSG_OUT(MessageType_MessageType_ButtonRequest,          ButtonRequest_fields,       NO_PROCESS_FUNC)
#if DEBUG_LINK
    // debug in messages
    DEBUG_IN(MessageType_MessageType_DebugLinkDecision,     DebugLinkDecision_fields,   NO_PROCESS_FUNC)
    DEBUG_IN(MessageType_MessageType_DebugLinkGetState,     DebugLinkGetState_fields,   (message_handler_t)(handler_debug_link_get_state))
    DEBUG_IN(MessageType_MessageType_DebugLinkStop,         DebugLinkStop_fields,       (message_handler_t)(handler_debug_link_stop))
    DEBUG_IN(MessageType_MessageType_DebugLinkFillConfig,   DebugLinkFillConfig_fields, (message_handler_t)(handler_debug_link_fill_config))
    // debug out messages
    DEBUG_OUT(MessageType_MessageType_DebugLinkState,       DebugLinkState_fields,      NO_PROCESS_FUNC)
    DEBUG_OUT(MessageType_MessageType_DebugLinkLog,         DebugLinkLog_fields,        NO_PROCESS_FUNC)
#endif
};

/*
 * check_firmware_hash - checks flashed firmware's hash
 *
 * INPUT
 *  - none
 *
 * OUTPUT
 *  - status of hash check
 */
static bool check_firmware_hash(void)
{
    uint8_t flashed_firmware_hash[SHA256_DIGEST_LENGTH];

    memory_firmware_hash(flashed_firmware_hash);

    return(memcmp(firmware_hash, flashed_firmware_hash, SHA256_DIGEST_LENGTH) == 0);
}

/*
 * bootloader_fsm_init() - initiliaze fsm for bootloader
 *
 * INPUT
 *  - none
 *
 * OUTPUT
 *  - update status
 */
static void bootloader_fsm_init(void)
{
    msg_map_init(MessagesMap, sizeof(MessagesMap) / sizeof(MessagesMap_t));
    set_msg_failure_handler(&send_failure);

#if DEBUG_LINK
    set_msg_debug_link_get_state_handler(&handler_debug_link_get_state);
#endif

    msg_init();
}

/*
 *  flash_locking_write - restore storage partition in flash for firmware update
 *
 *  INPUT -
 *      1. flash partition
 *      2. flash offset within partition to begin write
 *      3. length to write
 *      4. pointer to source data
 *
 *  OUTPUT -
 *      status
 */
static bool flash_locking_write(Allocation group, size_t offset, size_t len,
                         uint8_t *dataPtr)
{
    bool ret_val = true;

    flash_unlock();

    if(flash_write(group, offset, len, dataPtr) == false)
    {
        /* flash error detectected */
        ret_val = false;
    }

    flash_lock();
    return(ret_val);
}

/*
 * usb_flash_firmware() - update firmware over usb bus
 *
 * INPUT
 *  - none
 *
 * OUTPUT
 *  - update status
 */
bool usb_flash_firmware(void)
{
    bool ret_val = false;

    layout_warning("Firmware Update Mode");

    usb_init();
    bootloader_fsm_init();

    while(1)
    {
        switch(upload_state)
        {
            case UPLOAD_COMPLETE:
            {
                if(signatures_ok() == 1)
                {
                    /* The image is from KeepKey.  Restore storage data */
                    if(flash_locking_write(FLASH_STORAGE, 0, sizeof(ConfigFlash),
                                           (uint8_t *)&storage_shadow) == false)
                    {
                        /* bailing early */
                        goto uff_exit;
                    }
                }

                /* Check CRC of firmware that was flashed */
                if(check_firmware_hash())
                {
                    /* Fingerprint has been verified.  Install "KPKY" magic in meta header */
                    if(flash_locking_write(FLASH_APP, 0, META_MAGIC_SIZE, (uint8_t *)META_MAGIC_STR) == true)
                    {
                        send_success("Upload complete");
                        ret_val = true;
                    }
                }

                goto uff_exit;
            }

            case UPLOAD_ERROR:
            {
                dbg_print("error: Firmware update error...\n\r");
                goto uff_exit;
            }

            case UPLOAD_NOT_STARTED:
            case UPLOAD_STARTED:
            default:
            {
                usb_poll();
                animate();
                display_refresh();
            }
        }
    }

uff_exit:
    /* Clear the shadow before exiting */
    memset(&storage_shadow, 0, sizeof(ConfigFlash));
    return(ret_val);
}

/*
 * send_success() - Send success message over usb port
 *
 * INPUT
 *  - message string
 *
 * OUTPUT
 *  - none
 *
 */

void send_success(const char *text)
{
    RESP_INIT(Success);

    if(text)
    {
        resp.has_message = true;
        strlcpy(resp.message, text, sizeof(resp.message));
    }

    msg_write(MessageType_MessageType_Success, &resp);
}

/*
 * send_falure() -  Send failure message over usb port
 *
 * INPUT
 *  - failure code
 *  - message string
 *
 * OUTPUT
 *  - none
 *
 */
void send_failure(FailureType code, const char *text)
{
    if(reset_msg_stack)
    {
        handler_initialize((Initialize *)0);
        reset_msg_stack = false;
        return;
    }

    RESP_INIT(Failure);

    resp.has_code = true;
    resp.code = code;

    if(text)
    {
        resp.has_message = true;
        strlcpy(resp.message, text, sizeof(resp.message));
    }

    msg_write(MessageType_MessageType_Failure, &resp);
}

/*
 * handler_ping() - stubbed handler
 *
 * INPUT -
 *      none
 * OUTPUT -
 *      none
 */
void handler_ping(Ping *msg)
{
    RESP_INIT(Success);

    if(msg->has_message)
    {
        resp.has_message = true;
        memcpy(resp.message, &(msg->message), sizeof(resp.message));
    }

    msg_write(MessageType_MessageType_Success, &resp);
}

/*
 * handler_initialize() - handler to respond to usb host with bootloader
 *      initialization values
 *
 * INPUT -
 *      usb buffer - not used
 * OUTPUT -
 *      none
 */
void handler_initialize(Initialize *msg)
{
    (void)msg;
    RESP_INIT(Features);

    resp.has_bootloader_mode = true;
    resp.bootloader_mode = true;
    resp.has_major_version = true;
    resp.major_version = BOOTLOADER_MAJOR_VERSION;
    resp.minor_version = BOOTLOADER_MINOR_VERSION;
    resp.patch_version = BOOTLOADER_PATCH_VERSION;

    msg_write(MessageType_MessageType_Features, &resp);
}

/*
 * handler_erase() - The function is invoked by the host PC to erase "storage"
 *      and "application" partitions.
 *
 * INPUT -
 *    *msg = unused argument
 * OUTPUT - none
 *
 */
void handler_erase(FirmwareErase *msg)
{
    (void)msg;

    if(confirm(ButtonRequestType_ButtonRequest_FirmwareErase,
               "Verify Backup Before Upgrade",
               "Before upgrading, confirm that you have access to the backup of your recovery sentence."))
    {

        layout_simple_message("Preparing For Upgrade...");

        /* Save storage data in memory so it can be copied back after firmware update */
        memcpy(&storage_shadow, (void *)FLASH_STORAGE_START, sizeof(ConfigFlash));

        flash_unlock();
        flash_erase_word(FLASH_STORAGE);
        flash_erase_word(FLASH_APP);
        flash_lock();
        send_success("Firmware Erased");

        layout_loading();
    }
}

/*
 * raw_handler_upload() - The function updates application image downloaded over the USB
 *      to application partition.  Prior to the update, it will validate image
 *      SHA256 (finger print) and install "KPKY" magic upon validation
 *
 * INPUT -
 *     1. buffer pointer
 *     2. size of buffer
 *     3. size of file
 *
 * OUTPUT - none
 *
 */
void raw_handler_upload(uint8_t *msg, uint32_t msg_size, uint32_t frame_length)
{
    static uint32_t flash_offset;

    /* Check file size is within allocated space */
    if(frame_length < (FLASH_APP_LEN + FLASH_META_DESC_LEN))
    {
        /* Start firmware load */
        if(upload_state == UPLOAD_NOT_STARTED)
        {
            upload_state = UPLOAD_STARTED;
            flash_offset = 0;

            /*
             * Parse firmware hash
             */
            memcpy(firmware_hash, msg + PROTOBUF_FIRMWARE_HASH_START, SHA256_DIGEST_LENGTH);

            /*
             * Parse application start
             */
            msg_size -= PROTOBUF_FIRMWARE_START;
            msg = (uint8_t *)(msg + PROTOBUF_FIRMWARE_START);
        }

        /* Process firmware upload */
        if(upload_state == UPLOAD_STARTED)
        {
            /* Check if the image is bigger than allocated space */
            if((flash_offset + msg_size) < (FLASH_APP_LEN + FLASH_META_DESC_LEN))
            {
                if(flash_offset == 0)
                {
                    /* Check that image is prepared with KeepKey magic */
                    if(memcmp(msg, META_MAGIC_STR, META_MAGIC_SIZE) == 0)
                    {
                        msg_size -= META_MAGIC_SIZE;
                        msg = (uint8_t *)(msg + META_MAGIC_SIZE);
                        flash_offset = META_MAGIC_SIZE;
                        /* unlock the flash for writing */
                        flash_unlock();
                    }
                    else
                    {
                        /* Invalid KeepKey magic detected */
                        send_failure(FailureType_Failure_FirmwareError, "Not valid firmware");
                        upload_state = UPLOAD_ERROR;
                        dbg_print("Error: invalid Magic Key detected... \n\r");
                        goto rhu_exit;
                    }

                }

                /* Begin writing to flash */
                if(!flash_write(FLASH_APP, flash_offset, msg_size, msg))
                {
                    /* Error: flash write error */
                    flash_lock();
                    send_failure(FailureType_Failure_FirmwareError,
                                 "Encountered error while writing to flash");
                    upload_state = UPLOAD_ERROR;
                    dbg_print("Error: flash write error... \n\r");
                    goto rhu_exit;
                }

                flash_offset += msg_size;
            }
            else
            {
                /* Error: frame overrun detected during the image update */
                flash_lock();
                send_failure(FailureType_Failure_FirmwareError, "Firmware too large");
                upload_state = UPLOAD_ERROR;
                dbg_print("Error: frame overrun detected during the image update... \n\r");
                goto rhu_exit;
            }

            /* Finish firmware update */
            if(flash_offset >= frame_length - PROTOBUF_FIRMWARE_START)
            {
                flash_lock();
                upload_state = UPLOAD_COMPLETE;
            }
        }
    }
    else
    {
        send_failure(FailureType_Failure_FirmwareError, "Firmware too large");
        dbg_print("Error: image too large to fit in the allocated space : 0x%x ...\n\r",
                  frame_length);
        upload_state = UPLOAD_ERROR;
    }

rhu_exit:
    return;
}

#if DEBUG_LINK
/*
 * handler_debug_link_get_state() - Handler for debug link get state
 *
 * INPUT
 *  - msg:  pointer to DebugLinkGetState msg
 *
 * OUTPUT
 *  - none
 */
void handler_debug_link_get_state(DebugLinkGetState *msg)
{
    (void)msg;
    RESP_INIT(DebugLinkState);

    /* App fingerprint */
    if((resp.firmware_hash.size = memory_firmware_hash(resp.firmware_hash.bytes)) != 0)
    {
        resp.has_firmware_hash = true;
    }

    /* Storage fingerprint */
    resp.has_storage_hash = true;
    resp.storage_hash.size = memory_storage_hash(
                                 resp.storage_hash.bytes, sizeof(ConfigFlash));

    msg_debug_write(MessageType_MessageType_DebugLinkState, &resp);
}

/*
 * handler_debug_link_stop() - Handler for debug link stop
 *
 * INPUT
 *  - msg:  pointer to DebugLinkStop msg
 *
 * OUTPUT
 *  - none
 */
void handler_debug_link_stop(DebugLinkStop *msg)
{
    (void)msg;
}

/*
 * handler_debug_link_fill_config() - Fills config area with sample data (used for testing firmware upload)
 *
 * INPUT
 *  - msg:  pointer to DebugLinkFillConfig msg
 *
 * OUTPUT
 *  - none
 */
void handler_debug_link_fill_config(DebugLinkFillConfig *msg)
{
    (void)msg;

    ConfigFlash fill_storage_shadow;

    memset((uint8_t *)&fill_storage_shadow, 0xaa, sizeof(ConfigFlash));

    flash_locking_write(FLASH_STORAGE, 0, sizeof(ConfigFlash),
                        (uint8_t *)&fill_storage_shadow);
}
#endif
