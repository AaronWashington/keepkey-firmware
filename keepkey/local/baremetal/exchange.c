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
 */

/* === Includes ============================================================ */

#include <stdio.h>
#include <ecdsa.h>
#include <crypto.h>
#include <types.pb.h>
#include <bip32.h>
#include <coins.h>
#include <exchange.h>
#include <layout.h>
#include <app_confirm.h>
#include <msg_dispatch.h>

/* === Private Variables =================================================== */
/* exchange error variable */
static ExchangeError exchange_error = NO_EXCHANGE_ERROR;

/* exchange public key for signature varification */
static uint8_t exchange_pub_key[21] =
{
    0x00, 0x70, 0x92, 0xe3, 0x83, 0xde, 0x34, 0x8c, 0x02, 0x47, 
    0x39, 0xc1, 0x11, 0x4f, 0xa7, 0x9e, 0xeb, 0x97, 0xa2, 0x15, 
    0xcb
};

/* === Private Functions =================================================== */

/*
 * verify_exchange_address - verify address specified in exchange contract belongs to device.
 *
 * INPUT
 *     coin_name - name of coin
 *     address_n_count - depth of node
 *     address_n - pointer to node path
 *     address_str - string representation of address
 *     root - root hd node
 *
 * OUTPUT
 *     true/false - success/failure
 */
static bool verify_exchange_address(char *coin_name, size_t address_n_count,
                                    uint32_t *address_n, char *address_str, const HDNode *root)
{
    const CoinType *coin;
    HDNode node;
    char internal_address[36];
    bool ret_stat = false;

    coin = coinByName(coin_name);

    if(coin)
    {
    	memcpy(&node, root, sizeof(HDNode));
        if(hdnode_private_ckd_cached(&node, address_n, address_n_count) != 0)
        {
            ecdsa_get_address(node.public_key, coin->address_type, internal_address,
                              sizeof(internal_address));

            if(strncmp(internal_address, address_str, sizeof(internal_address)) == 0)
            {
                ret_stat = true;
            }
        }
    }

    return(ret_stat);
}

/*
 *  get_response_coin() - get pointer to coin type 
 *
 * INPUT
 *     response_coin_short_name: pointer to abbreviated coin name
 * OUTPUT
 *     pointer to coin type 
 *     NULL: error
 */
const CoinType * get_response_coin(const char *response_coin_short_name)
{
    char local_coin_name[17];

    strlcpy(local_coin_name, response_coin_short_name, sizeof(local_coin_name));
    strupr(local_coin_name);
    return(coinByShortcut((const char *)local_coin_name));
}

/*
 * verify_exchange_coin() - Verify coin type in exchange contract
 *
 * INPUT
 *     coin1: reference coin name
 *     coin2: coin name in exchange
 *     len: length of buffer
 * OUTPUT
 *     true/false -  success/failure
 */
bool verify_exchange_coin(const char *coin1, const char *coin2, uint32_t len)
{
    bool ret_stat = false;
    const CoinType *response_coin = get_response_coin(coin2);

    if(response_coin != 0)
    {
        if(strncmp(coin1, response_coin->coin_name, len) == 0)
        {
            ret_stat = true;
        }
    }
    return(ret_stat);
}
/*
 * verify_exchange_contract() - Verify content of exchange contract is valid
 *
 * INPUT
 *     exchange:  exchange pointer
 *     root - root hd node
 * OUTPUT
 *     true/false -  success/failure
 */
static bool verify_exchange_contract(const CoinType *coin, TxOutputType *tx_out, const HDNode *root)
{
    bool ret_stat = false;
    int response_raw_filled_len = 0; 
    uint8_t response_raw[sizeof(ExchangeResponse)];
    const CoinType *response_coin = NULL, *signed_coin = NULL;
    ExchangeType *exchange = &tx_out->exchange_type;
    
    /* verify Exchange signature */
    memset(response_raw, 0, sizeof(response_raw));
    response_raw_filled_len = encode_pb(
                                (const void *)&exchange->signed_exchange_response.response, 
                                ExchangeResponse_fields,
                                response_raw, 
                                sizeof(response_raw));

    if(response_raw_filled_len != 0)
    {
        signed_coin = coinByShortcut((const char *)"BTC");
        if(cryptoMessageVerify(signed_coin, response_raw, response_raw_filled_len, exchange_pub_key, 
                    (uint8_t *)exchange->signed_exchange_response.signature.bytes) != 0)
        {
            set_exchange_error(ERROR_EXCHANGE_SIGNATURE);
            goto verify_exchange_contract_exit;
        }
    }
    else
    {
        set_exchange_error(ERROR_EXCHANGE_SIGNATURE);
        goto verify_exchange_contract_exit;
    }

    /* verify Deposit coin type */
    if(!verify_exchange_coin(coin->coin_name,
                         exchange->signed_exchange_response.response.deposit_address.coin_type,
                         sizeof(coin->coin_name)))
    {
        set_exchange_error(ERROR_EXCHANGE_DEPOSIT_COINTYPE);
        goto verify_exchange_contract_exit;
    }
    /* verify Deposit address */
    if(strncmp(tx_out->address, 
               exchange->signed_exchange_response.response.deposit_address.address, 
               sizeof(tx_out->address)) != 0)
    {
        set_exchange_error(ERROR_EXCHANGE_DEPOSIT_ADDRESS);
        goto verify_exchange_contract_exit;
    }

    /* verify Deposit amount*/
    if(tx_out->amount != exchange->signed_exchange_response.response.deposit_amount)
    {
        set_exchange_error(ERROR_EXCHANGE_DEPOSIT_AMOUNT);
        goto verify_exchange_contract_exit;
    }

    /* verify Withdrawal coin type */
    if(!verify_exchange_coin(exchange->withdrawal_coin_name,
             exchange->signed_exchange_response.response.withdrawal_address.coin_type,
             sizeof(exchange->withdrawal_coin_name)))
    {
        set_exchange_error(ERROR_EXCHANGE_WITHDRAWAL_COINTYPE);
        goto verify_exchange_contract_exit;
    }

    /* verify Withdrawal address */
    if(!verify_exchange_address( exchange->withdrawal_coin_name,
             exchange->withdrawal_address_n_count,
             exchange->withdrawal_address_n,
             exchange->signed_exchange_response.response.withdrawal_address.address, root))
    {
        set_exchange_error(ERROR_EXCHANGE_WITHDRAWAL_ADDRESS);
        goto verify_exchange_contract_exit;
    }

    /* verify Return coin type */
    if(!verify_exchange_coin(coin->coin_name,
             exchange->signed_exchange_response.response.return_address.coin_type,
             sizeof(coin->coin_name)))
    {
        set_exchange_error(ERROR_EXCHANGE_RETURN_COINTYPE);
        goto verify_exchange_contract_exit;
    }
    /* verify Return address */

    response_coin = get_response_coin(exchange->signed_exchange_response.response.return_address.coin_type);
    if(!verify_exchange_address( (char *)response_coin->coin_name,
             exchange->return_address_n_count,
             exchange->return_address_n,
             exchange->signed_exchange_response.response.return_address.address, root))
    {
        set_exchange_error(ERROR_EXCHANGE_RETURN_ADDRESS);
    }
    else
    {
        set_exchange_error(NO_EXCHANGE_ERROR);
        ret_stat = true;
    }

verify_exchange_contract_exit:
    return(ret_stat);
}

/* === Functions =========================================================== */

/*
 * set_exchange_error - set exchange error code
 * INPUT 
 *     error_code - exchange error code  
 * OUTPUT
 *     none 
 */
void set_exchange_error(ExchangeError error_code)
{
    exchange_error = error_code;
}
/*
 * get_exchange_error - get exchange error code
 * INPUT 
 *     none
 * OUTPUT
 *     exchange error code  
 */
ExchangeError get_exchange_error(void)
{
    return(exchange_error);
}

/*
 * process_exchange_contract() - validate contract from exchange and populate the transaction
 *                            output structure
 *
 * INPUT
 *      coin - pointer signTx coin type
 *      tx_out - pointer transaction output structure
 *      root - root hd node
 *      needs_confirm - whether requires user manual approval
 * OUTPUT
 *      true/false - success/failure
 */
bool process_exchange_contract(const CoinType *coin, TxOutputType *tx_out, const HDNode *root, bool needs_confirm)
{
    bool ret_val = false;
    const CoinType *withdraw_coin, *deposit_coin;
    char amount_from_str[32], amount_to_str[32], node_str[100];

    if(tx_out->has_exchange_type)
    {
        /* validate contract before processing */
        if(verify_exchange_contract(coin, tx_out, root))
        {
            withdraw_coin = coinByName(tx_out->exchange_type.withdrawal_coin_name);
            deposit_coin = get_response_coin(tx_out->exchange_type.signed_exchange_response.response.deposit_address.coin_type);

            if(needs_confirm)
            {
                coin_amnt_to_str(deposit_coin, tx_out->exchange_type.signed_exchange_response.response.deposit_amount, amount_from_str, sizeof(amount_to_str));
                coin_amnt_to_str(withdraw_coin, tx_out->exchange_type.signed_exchange_response.response.withdrawal_amount, amount_to_str, sizeof(amount_from_str));
                node_path_to_string(withdraw_coin, node_str, tx_out->exchange_type.withdrawal_address_n,
                         tx_out->exchange_type.withdrawal_address_n_count);

                if(!confirm_exchange_output("ShapeShift", amount_from_str, amount_to_str, node_str))
                {
                    goto process_exchange_contract_exit;
                }
            }

            ret_val = true;
        }
    } 

process_exchange_contract_exit:
    return ret_val;
}

