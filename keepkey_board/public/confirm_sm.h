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
/*
 * @brief General confirmation state machine
 */

#ifndef CONFIRM_SM_H
#define CONFIRM_SM_H

#include <stdbool.h>

#include <interface.h>

/**
 * @param request The string to display for confirmation.
 * @param varargs for the request, printf style.
 *
 * @return true on confirmation.
 *
 * @note The timeout is currently fixed.
 */

bool confirm_with_button_request(ButtonRequestType type, const char *request_title, const char *request_body, ...);
bool confirm(const char* request_title, const char* request_body, ...);
bool review(const char* request_title, const char* request_body, ...);
bool confirm_helper(const char* request_title, const char* request_body);
void cancel_confirm(FailureType code, const char *text);
void success_confirm(const char *text);
bool confirm_cipher(bool encrypt, const char *key);
bool confirm_encrypt_msg(const char *msg, bool signing);
bool confirm_decrypt_msg(const char *msg, const char *address);
bool confirm_ping_msg(const char *msg);
bool confirm_transaction_output(const char *amount, const char *to);

#endif
