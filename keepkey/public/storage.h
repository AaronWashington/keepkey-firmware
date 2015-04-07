/*
 * This file is part of the TREZOR project.
 *
 * Copyright (C) 2014 Pavol Rusnak <stick@satoshilabs.com>
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
 *
 * Jan 12, 2015 - This file has been modified and adapted for KeepKey project.
 *
 */

#ifndef __STORAGE_H__
#define __STORAGE_H__

#include "types.pb.h"
#include "storage.pb.h"
#include "messages.pb.h"
#include "bip32.h"
#include "memory.h"

/**
 * Initialize internal storage and configuration.
 *
 * If this function fails to find a valid config block 
 * it will blow away and reinitialize the current one.
 */

/*************  Defines  **************************/
#define STORAGE_VERSION 1

/*************  Typedefs and Enums *****************/

/*************  Function declarations **************/

void storage_init(void);
void storage_reset_uuid(void);
void storage_reset(void);
void session_clear(bool clear_pin);
void storage_commit(void);

void storage_loadDevice(LoadDevice *msg);

bool storage_getRootNode(HDNode *node);

void storage_setLabel(const char *label);

const char *storage_getLanguage(void);
void storage_setLanguage(const char *lang);

void session_cachePassphrase(const char *passphrase);
bool session_isPassphraseCached(void);

bool storage_is_pin_correct(const char *pin);
bool storage_has_pin(void);
void storage_set_pin(const char *pin);
const char* storage_get_pin(void);
void session_cache_pin(void);
bool session_is_pin_cached(void);
void storage_reset_pin_fails(void);
void storage_increase_pin_fails(void);
uint32_t storage_get_pin_fails(void);

bool storage_isInitialized(void);

const char* storage_get_uuid_str(void);
const char* storage_get_language(void);
const char* storage_get_label(void);

bool storage_get_passphrase_protected(void);
void storage_set_passphrase_protected(bool p);
void storage_set_mnemonic_from_words(const char (*words)[12], unsigned int num_words);
void storage_set_mnemonic(const char *mnemonic);
bool storage_has_mnemonic(void);

const char* storage_get_mnemonic(void);
const char* storage_get_shadow_mnemonic(void);

bool storage_get_imported(void);

bool storage_has_node(void);
HDNodeType* storage_get_node(void);

#endif
