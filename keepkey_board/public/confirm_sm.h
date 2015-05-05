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

#ifndef CONFIRM_SM_H
#define CONFIRM_SM_H

#include <stdbool.h>
#include <interface.h>

/***************** #defines ******************************/
/* The number of milliseconds to wait for a confirmation */
#define CONFIRM_TIMEOUT_MS 1200

#define CONFIRM_SIGN_IDENTITY_TITLE 32
#define CONFIRM_SIGN_IDENTITY_BODY 416

/***************** typedefs and enums  *******************/
typedef enum
{
    HOME,
    CONFIRM_WAIT,
    CONFIRMED,
    FINISHED
} DisplayState;

typedef enum
{
    LAYOUT_REQUEST,
	LAYOUT_REQUEST_NO_ANIMATION,
    LAYOUT_CONFIRM_ANIMATION,
    LAYOUT_CONFIRMED,
    LAYOUT_FINISHED,
    LAYOUT_NUM_LAYOUTS,
    LAYOUT_INVALID
} ActiveLayout;

/* Define the given layout dialog texts for each screen */
typedef struct
{
    const char* request_title;
    const char* request_body;
} ScreenLine;

typedef ScreenLine ScreenLines;
typedef ScreenLines DialogLines[LAYOUT_NUM_LAYOUTS];

typedef struct 
{
    DialogLines lines;
    DisplayState display_state;
    ActiveLayout active_layout;
} StateInfo;

typedef void (*layout_notification_t)(const char* str1, const char* str2, NotificationType type);

/******************* Function Declarations *****************************/
bool confirm(ButtonRequestType type, const char *request_title, const char *request_body, ...);
bool confirm_with_custom_layout(layout_notification_t layout_notification_func, ButtonRequestType type, 
    const char *request_title, const char *request_body, ...);
bool confirm_without_button_request(const char* request_title, const char* request_body, ...);
bool review(ButtonRequestType type, const char* request_title, const char* request_body, ...);
bool review_without_button_request(const char *request_title, const char *request_body, ...);
bool confirm_helper(const char* request_title, const char* request_body, layout_notification_t layout_notification_func);
bool confirm_cipher(bool encrypt, const char *key);
bool confirm_encrypt_msg(const char *msg, bool signing);
bool confirm_decrypt_msg(const char *msg, const char *address);
bool confirm_transaction_output(const char *amount, const char *to);
bool confirm_load_device(bool is_node);
bool confirm_address(const char *desc, const char *address);
bool confirm_sign_identity(const IdentityType *identity, const char *challenge);

#endif
