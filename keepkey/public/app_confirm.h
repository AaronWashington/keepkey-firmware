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

#ifndef APP_CONFIRM_H
#define APP_CONFIRM_H

#include <stdbool.h>
#include <interface.h>

/***************** #defines ******************************/

#define CONFIRM_SIGN_IDENTITY_TITLE 32
#define CONFIRM_SIGN_IDENTITY_BODY 416

/***************** typedefs and enums  *******************/

/******************* Function Declarations *****************************/

bool confirm_cipher(bool encrypt, const char *key);
bool confirm_encrypt_msg(const char *msg, bool signing);
bool confirm_decrypt_msg(const char *msg, const char *address);
bool confirm_transaction_output(const char *amount, const char *to);
bool confirm_load_device(bool is_node);
bool confirm_address(const char *desc, const char *address);
bool confirm_sign_identity(const IdentityType *identity, const char *challenge);

#endif // APP_CONFIRM_H