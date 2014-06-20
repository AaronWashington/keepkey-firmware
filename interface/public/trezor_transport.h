
/* START KEEPKEY LICENSE */
/*
 * This file is part of the KeepKey project.
 *
 * Copyright (C) 2014 Carbon Design Group <tom@carbondesign.com>
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

#ifndef KEEPKEY_TRANSPORT_H
#define KEEPKEY_TRANSPORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FRAME_SIZE 1024

#pragma pack(1)

/**
 * This structure is derived from the Trezor protocol.  Note that the values come in as big endian, so
 * they'll need to be swapped.
 */
typedef struct
{
    uint8_t hid_type;  // This is always '?', and is part of the Trezor protocol.
} UsbHeader;

/**
 * Trezor frame header
 */
typedef struct
{
    /* 
     * Not sure what these are for.  They are derived from the Trezor code. I think they denote the first
     * USB segment in a message, in the case where multiple USB segments are sent. 
     */
    uint8_t pre1;
    uint8_t pre2;

    /* Protobuf ID */
    uint16_t id;    

    /* Length of the following message */
    uint32_t len;

} TrezorFrameHeader;

typedef struct 
{
    UsbHeader usb_header;
    TrezorFrameHeader header;
    uint8_t contents[0];
} TrezorFrame;

typedef struct
{
    TrezorFrame frame;
    uint8_t buffer[MAX_FRAME_SIZE];
} TrezorFrameBuffer;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif
