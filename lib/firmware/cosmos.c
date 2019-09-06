/*
 * This file is part of the TREZOR project.
 *
 * Copyright (C) 2016 Alex Beregszaszi <alex@rtfs.hu>
 * Copyright (C) 2016 Pavol Rusnak <stick@satoshilabs.com>
 * Copyright (C) 2016 Jochen Hoenicke <hoenicke@gmail.com>
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

#include "keepkey/firmware/cosmos.h"

#include "keepkey/board/confirm_sm.h"
#include "keepkey/board/layout.h"
#include "keepkey/board/util.h"
#include "keepkey/firmware/app_confirm.h"
#include "keepkey/firmware/coins.h"
#include "keepkey/firmware/crypto.h"
#include "keepkey/firmware/fsm.h"
#include "keepkey/firmware/home_sm.h"
#include "keepkey/firmware/cosmos_contracts/makerdao.h"
#include "keepkey/firmware/cosmos_tokens.h"
#include "keepkey/firmware/storage.h"
#include "keepkey/firmware/transaction.h"
#include "trezor/crypto/address.h"
#include "trezor/crypto/ecdsa.h"
#include "trezor/crypto/memzero.h"
#include "trezor/crypto/secp256k1.h"
//#include "trezor/crypto/sha3.h"

#include <stdio.h>

#define _(X) (X)

#define MAX_CHAIN_ID 2147483630

static bool cosmos_signing = false;
static uint32_t data_total, data_left;
static CosmosTxRequest msg_tx_request;
static CONFIDENTIAL uint8_t privkey[32];
static uint32_t chain_id;
static uint32_t tx_type;
struct SHA3_CTX keccak_ctx_2;

static inline void hash_data(const uint8_t *buf, size_t size)
{
    sha3_Update(&keccak_ctx_2, buf, size);
}

/*
 * Push an RLP encoded length to the hash buffer.
 */
static void hash_rlp_length(uint32_t length, uint8_t firstbyte)
{
    uint8_t buf[4];
    if (length == 1 && firstbyte <= 0x7f) {
        /* empty length header */
    } else if (length <= 55) {
        buf[0] = 0x80 + length;
        hash_data(buf, 1);
    } else if (length <= 0xff) {
        buf[0] = 0xb7 + 1;
        buf[1] = length;
        hash_data(buf, 2);
    } else if (length <= 0xffff) {
        buf[0] = 0xb7 + 2;
        buf[1] = length >> 8;
        buf[2] = length & 0xff;
        hash_data(buf, 3);
    } else {
        buf[0] = 0xb7 + 3;
        buf[1] = length >> 16;
        buf[2] = length >> 8;
        buf[3] = length & 0xff;
        hash_data(buf, 4);
    }
}

/*
 * Push an RLP encoded list length to the hash buffer.
 */
static void hash_rlp_list_length(uint32_t length)
{
    uint8_t buf[4];
    if (length <= 55) {
        buf[0] = 0xc0 + length;
        hash_data(buf, 1);
    } else if (length <= 0xff) {
        buf[0] = 0xf7 + 1;
        buf[1] = length;
        hash_data(buf, 2);
    } else if (length <= 0xffff) {
        buf[0] = 0xf7 + 2;
        buf[1] = length >> 8;
        buf[2] = length & 0xff;
        hash_data(buf, 3);
    } else {
        buf[0] = 0xf7 + 3;
        buf[1] = length >> 16;
        buf[2] = length >> 8;
        buf[3] = length & 0xff;
        hash_data(buf, 4);
    }
}

/*
 * Push an RLP encoded length field and data to the hash buffer.
 */
static void hash_rlp_field(const uint8_t *buf, size_t size)
{
    hash_rlp_length(size, buf[0]);
    hash_data(buf, size);
}

/*
 * Push an RLP encoded number to the hash buffer.
 * Cosmos yellow paper says to convert to big endian and strip leading zeros.
 */
static void hash_rlp_number(uint32_t number)
{
    if (!number) {
        return;
    }
    uint8_t data[4];
    data[0] = (number >> 24) & 0xff;
    data[1] = (number >> 16) & 0xff;
    data[2] = (number >> 8) & 0xff;
    data[3] = (number) & 0xff;
    int offset = 0;
    while (!data[offset]) {
        offset++;
    }
    hash_rlp_field(data + offset, 4 - offset);
}

/*
 * Calculate the number of bytes needed for an RLP length header.
 * NOTE: supports up to 16MB of data (how unlikely...)
 * FIXME: improve
 */
static int rlp_calculate_length(int length, uint8_t firstbyte)
{
    if (length == 1 && firstbyte <= 0x7f) {
        return 1;
    } else if (length <= 55) {
        return 1 + length;
    } else if (length <= 0xff) {
        return 2 + length;
    } else if (length <= 0xffff) {
        return 3 + length;
    } else {
        return 4 + length;
    }
}

static int rlp_calculate_number_length(uint32_t number)
{
    if (number <= 0x7f) {
        return 1;
    }
    else if (number <= 0xff) {
        return 2;
    }
    else if (number <= 0xffff) {
        return 3;
    }
    else if (number <= 0xffffff) {
        return 4;
    } else {
        return 5;
    }
}

static void send_request_chunk(void)
{
    layoutProgress(_("Cosmos Signing"), (data_total - data_left) * 1000 / data_total);
    msg_tx_request.has_data_length = true;
    msg_tx_request.data_length = data_left <= 1024 ? data_left : 1024;
    msg_write(MessageType_MessageType_CosmosTxRequest, &msg_tx_request);
}

static int cosmos_is_canonic(uint8_t v, uint8_t signature[64])
{
    (void) signature;
    return (v & 2) == 0;
}

static void send_signature(void)
{
    uint8_t hash[32], sig[64];
    uint8_t v;
    layoutProgress(_("Signing"), 1000);

    /* eip-155 replay protection */
    if (chain_id) {
        /* hash v=chain_id, r=0, s=0 */
        hash_rlp_number(chain_id);
        hash_rlp_length(0, 0);
        hash_rlp_length(0, 0);
    }

    keccak_Final(&keccak_ctx_2, hash);
    if (ecdsa_sign_digest(&secp256k1, privkey, hash, sig, &v, cosmos_is_canonic) != 0) {
        fsm_sendFailure(FailureType_Failure_Other, "Cosmos Signing failed");
        cosmos_signing_abort();
        return;
    }

    memzero(privkey, sizeof(privkey));

    /* Send back the result */
    msg_tx_request.has_data_length = false;

    msg_tx_request.has_signature_v = true;
    if (chain_id > MAX_CHAIN_ID) {
        msg_tx_request.signature_v = v;
    } else if (chain_id) {
        msg_tx_request.signature_v = v + 2 * chain_id + 35;
    } else {
        msg_tx_request.signature_v = v + 27;
    }

    msg_tx_request.has_signature_r = true;
    msg_tx_request.signature_r.size = 32;
    memcpy(msg_tx_request.signature_r.bytes, sig, 32);

    msg_tx_request.has_signature_s = true;
    msg_tx_request.signature_s.size = 32;
    memcpy(msg_tx_request.signature_s.bytes, sig + 32, 32);

    // KeepKey custom (for the KeepKey Client)
    msg_tx_request.has_hash = true;
    msg_tx_request.hash.size = sizeof(msg_tx_request.hash.bytes);
    memcpy(msg_tx_request.hash.bytes, hash, msg_tx_request.hash.size);
    msg_tx_request.has_signature_der = true;
    msg_tx_request.signature_der.size = ecdsa_sig_to_der(sig, msg_tx_request.signature_der.bytes);

    msg_write(MessageType_MessageType_CosmosTxRequest, &msg_tx_request);

    cosmos_signing_abort();
}
/* Format a 256 bit number (amount in wei) into a human readable format
 * using standard cosmos units.
 * The buffer must be at least 25 bytes.
 */
void cosmosFormatAmount(const bignum256 *amnt, const TokenType *token, uint32_t cid, char *buf, int buflen)
{
    bignum256 bn1e9;
    bn_read_uint32(1000000000, &bn1e9);
    const char *suffix = NULL;
    int decimals = 18;
    if (token == UnknownToken) {
        strlcpy(buf, "Unknown token value", buflen);
        return;
    } else
    if (token != NULL) {
        suffix = token->ticker;
        decimals = token->decimals;
    } else
    if (bn_is_less(amnt, &bn1e9)) {
        suffix = " Wei";
        decimals = 0;
    } else {
        if (tx_type == 1 || tx_type == 6) {
            suffix = " WAN";
        } else {
            // constants from trezor-common/defs/cosmos/networks.json
            switch (cid) {
                case    1: suffix = " ETH";  break;  // Cosmos
                case    2: suffix = " EXP";  break;  // Expanse
                case    3: suffix = " tROP"; break;  // Cosmos Testnet Ropsten
                case    4: suffix = " tRIN"; break;  // Cosmos Testnet Rinkeby
                case    8: suffix = " UBQ";  break;  // UBIQ
                case   20: suffix = " EOSC"; break;  // EOS Classic
                case   28: suffix = " ETSC"; break;  // Cosmos Social
                case   30: suffix = " RBTC"; break;  // RSK
                case   31: suffix = " tRBTC";break;  // RSK Testnet
                case   42: suffix = " tKOV"; break;  // Cosmos Testnet Kovan
                case   61: suffix = " ETC";  break;  // Cosmos Classic
                case   62: suffix = " tETC"; break;  // Cosmos Classic Testnet
                case   64: suffix = " ELLA"; break;  // Ellaism
                case  820: suffix = " CLO";  break;  // Callisto
                case 1987: suffix = " EGEM"; break;  // EtherGem
                default  : suffix = " UNKN"; break;  // unknown chain
            }
        }
    }
    bn_format(amnt, NULL, suffix, decimals, 0, false, buf, buflen);
}

static void layoutCosmosConfirmTx(const uint8_t *to, uint32_t to_len, const uint8_t *value, uint32_t value_len, const TokenType *token,
                                    char *out_str, size_t out_str_len, bool approve)
{
    bignum256 val;
    uint8_t pad_val[32];
    memset(pad_val, 0, sizeof(pad_val));
    memcpy(pad_val + (32 - value_len), value, value_len);
    bn_read_be(pad_val, &val);

    char amount[32];
    if (token == NULL) {
        if (bn_is_zero(&val)) {
            strcpy(amount, _("message"));
        } else {
            cosmosFormatAmount(&val, NULL, chain_id, amount, sizeof(amount));
        }
    } else {
        cosmosFormatAmount(&val, token, chain_id, amount, sizeof(amount));
    }

    char addr[43] = "0x";
    if (to_len) {
        ethereum_address_checksum(to, addr + 2, false, chain_id);
    }

    bool approve_all = approve && value_len == 32 &&
                       memcmp(value,      "\xff\xff\xff\xff\xff\xff\xff\xff", 8) == 0 &&
                       memcmp(value + 8,  "\xff\xff\xff\xff\xff\xff\xff\xff", 8) == 0 &&
                       memcmp(value + 16, "\xff\xff\xff\xff\xff\xff\xff\xff", 8) == 0 &&
                       memcmp(value + 24, "\xff\xff\xff\xff\xff\xff\xff\xff", 8) == 0;

    const char *address = addr;
    if (to_len && makerdao_isOasisDEXAddress(to, chain_id)) {
        address = "OasisDEX";
    }

    int cx;
    if (approve && bn_is_zero(&val) && token) {
        cx = snprintf(out_str, out_str_len, "Remove ability for %s to withdraw %s?", address, token->ticker + 1);
    } else if (approve_all) {
        cx = snprintf(out_str, out_str_len, "Unlock full %s balance for withdrawal by %s?", token->ticker + 1, address);
    } else if (approve) {
        cx = snprintf(out_str, out_str_len, "Approve withdrawal of up to %s by %s?", amount, address);
    } else {
        cx = snprintf(out_str, out_str_len, "Send %s to %s", amount, to_len ? address : "new contract?");
    }

    if (out_str_len <= (size_t)cx) {
        /*error detected. Clear the buffer */
        memset(out_str, 0, out_str_len);
    }
}

static void layoutCosmosData(const uint8_t *data, uint32_t len, uint32_t total_len,
                               char *out_str, size_t out_str_len)
{
    char hexdata[3][17];
    char summary[20];
    uint32_t printed = 0;
    for (int i = 0; i < 3; i++) {
        uint32_t linelen = len - printed;
        if (linelen > 8) {
            linelen = 8;
        }
        data2hex(data, linelen, hexdata[i]);
        data += linelen;
        printed += linelen;
    }

    strcpy(summary, "...          bytes");
    char *p = summary + 11;
    uint32_t number = total_len;
    while (number > 0) {
        *p-- = '0' + number % 10;
        number = number / 10;
    }
    char *summarystart = summary;
    if (total_len == printed)
        summarystart = summary + 4;

    if((uint32_t)snprintf(out_str, out_str_len, "%s%s\n%s%s", hexdata[0], hexdata[1],
                          hexdata[2], summarystart) >= out_str_len) {
        /*error detected.  Clear the buffer */
        memset(out_str, 0, out_str_len);
    }
}

static void layoutCosmosFee(const uint8_t *value, uint32_t value_len,
                              const uint8_t *gas_price, uint32_t gas_price_len,
                              const uint8_t *gas_limit, uint32_t gas_limit_len,
                              bool is_token, char *out_str, size_t out_str_len)
{
    bignum256 val, gas;
    uint8_t pad_val[32];
    char tx_value[32];
    char gas_value[32];

    memzero(tx_value, sizeof(tx_value));
    memzero(gas_value, sizeof(gas_value));

    memset(pad_val, 0, sizeof(pad_val));
    memcpy(pad_val + (32 - gas_price_len), gas_price, gas_price_len);
    bn_read_be(pad_val, &val);

    memset(pad_val, 0, sizeof(pad_val));
    memcpy(pad_val + (32 - gas_limit_len), gas_limit, gas_limit_len);
    bn_read_be(pad_val, &gas);
    bn_multiply(&val, &gas, &secp256k1.prime);

    cosmosFormatAmount(&gas, NULL, chain_id, gas_value, sizeof(gas_value));

    memset(pad_val, 0, sizeof(pad_val));
    memcpy(pad_val + (32 - value_len), value, value_len);
    bn_read_be(pad_val, &val);

    if (bn_is_zero(&val)) {
        strcpy(tx_value, is_token ? _("the tokens") : _("the message"));
    } else {
        cosmosFormatAmount(&val, NULL, chain_id, tx_value, sizeof(tx_value));
    }

    if((uint32_t)snprintf(out_str, out_str_len,
                          _("Send %s from your wallet, paying up to %s for cosmos gas?"),
                          tx_value, gas_value) >= out_str_len) {
        /*error detected.  Clear the buffer */
        memset(out_str, 0, out_str_len);
    }
}


const CoinType *fsm_getCoin(bool has_name, const char *name)
{
    const CoinType *coin;
    if (has_name) {
        coin = coinByName(name);
    } else {
        coin = coinByName("Bitcoin");
    }
    if(!coin)
    {
        fsm_sendFailure(FailureType_Failure_Other, "Invalid coin name");
        layoutHome();
        return 0;
    }

    return coin;
}

/*
 * RLP fields:
 * - nonce (0 .. 32)
 * - gas_price (0 .. 32)
 * - gas_limit (0 .. 32)
 * - to (0, 20)
 * - value (0 .. 32)
 * - data (0 ..)
 */

static bool cosmos_signing_check(CosmosSignTx *msg)
{
    if (!msg->has_gas_price || !msg->has_gas_limit) {
        return false;
    }

    if (msg->to.size != 20 && msg->to.size != 0) {
        /* Address has wrong length */
        return false;
    }

    // sending transaction to address 0 (contract creation) without a data field
    if (msg->to.size == 0 && (!msg->has_data_length || msg->data_length == 0)) {
        return false;
    }

    if (msg->gas_price.size + msg->gas_limit.size  > 30) {
        // sanity check that fee doesn't overflow
        return false;
    }

    return true;
}

void cosmos_signing_init(CosmosSignTx *msg, const HDNode *node, bool needs_confirm)
{
    cosmos_signing = true;
    sha3_256_Init(&keccak_ctx_2);

    bool is_approve = false;

    memset(&msg_tx_request, 0, sizeof(CosmosTxRequest));
    /* set fields to 0, to avoid conditions later */
    if (!msg->has_value)
        msg->value.size = 0;
    if (!msg->has_data_initial_chunk)
        msg->data_initial_chunk.size = 0;
    if (!msg->has_to)
        msg->to.size = 0;
    if (!msg->has_nonce)
        msg->nonce.size = 0;


    if (msg->has_data_length && msg->data_length > 0) {
        if (!msg->has_data_initial_chunk || msg->data_initial_chunk.size == 0) {
            fsm_sendFailure(FailureType_Failure_Other, _("Data length provided, but no initial chunk"));
            cosmos_signing_abort();
            return;
        }
        /* Our encoding only supports transactions up to 2^24 bytes.  To
         * prevent exceeding the limit we use a stricter limit on data length.
         */
        if (msg->data_length > 16000000)  {
            fsm_sendFailure(FailureType_Failure_SyntaxError, _("Data length exceeds limit"));
            cosmos_signing_abort();
            return;
        }
        data_total = msg->data_length;
    } else {
        data_total = 0;
    }
    if (msg->data_initial_chunk.size > data_total) {
        fsm_sendFailure(FailureType_Failure_Other, _("Invalid size of initial chunk"));
        cosmos_signing_abort();
        return;
    }

    const TokenType *token = NULL;


    // safety checks
    if (!cosmos_signing_check(msg)) {
        fsm_sendFailure(FailureType_Failure_SyntaxError, _("Safety check failed"));
        cosmos_signing_abort();
        return;
    }

    bool data_needs_confirm = true;

    char confirm_body_message[BODY_CHAR_MAX];
    if (needs_confirm) {
        memset(confirm_body_message, 0, sizeof(confirm_body_message));

        layoutCosmosConfirmTx(msg->to.bytes, msg->to.size, msg->value.bytes, msg->value.size, NULL,
                                confirm_body_message, sizeof(confirm_body_message), /*approve=*/false);

        bool is_transfer = msg->address_type == OutputAddressType_TRANSFER;
        const char *title;
        ButtonRequestType BRT;
        if (is_approve) {
            title = "Approve";
            BRT = ButtonRequestType_ButtonRequest_ConfirmOutput;
        } else if (is_transfer) {
            title = "Transfer";
            BRT = ButtonRequestType_ButtonRequest_ConfirmTransferToAccount;
        } else {
            title = "Send";
            BRT = ButtonRequestType_ButtonRequest_ConfirmOutput;
        }
        if (!confirm(BRT, title, "%s", confirm_body_message)) {
            fsm_sendFailure(FailureType_Failure_ActionCancelled, "Signing cancelled by user");
            cosmos_signing_abort();
            return;
        }
    }

    memset(confirm_body_message, 0, sizeof(confirm_body_message));
    if (token == NULL && data_total > 0 && data_needs_confirm) {
        // KeepKey custom: warn the user that they're trying to do something
        // that is potentially dangerous. People (generally) aren't great at
        // parsing raw transaction data, and we can't effectively show them
        // what they're about to do in the general case.
        if (!storage_isPolicyEnabled("AdvancedMode")) {
            (void)review(ButtonRequestType_ButtonRequest_Other, "Warning",
                         "Signing of arbitrary Cosmos contract data is recommended only for "
                         "experienced users. Enable 'AdvancedMode' policy to dismiss.");
        }

        layoutCosmosData(msg->data_initial_chunk.bytes, msg->data_initial_chunk.size, data_total,
                           confirm_body_message, sizeof(confirm_body_message));
        if (!confirm(ButtonRequestType_ButtonRequest_ConfirmOutput, "Confirm Cosmos Data", "%s",
                     confirm_body_message)) {
            fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
            cosmos_signing_abort();
            return;
        }
    }

    memset(confirm_body_message, 0, sizeof(confirm_body_message));
    layoutCosmosFee(msg->value.bytes, msg->value.size,
                      msg->gas_price.bytes, msg->gas_price.size,
                      msg->gas_limit.bytes, msg->gas_limit.size, token != NULL,
                      confirm_body_message, sizeof(confirm_body_message));
    if(!confirm(ButtonRequestType_ButtonRequest_SignTx, "Transaction", "%s",
                confirm_body_message)) {
        fsm_sendFailure(FailureType_Failure_ActionCancelled, "Signing cancelled by user");
        cosmos_signing_abort();
        return;
    }

    /*
     *   BEGIN cosmos inject
     */

    const char *TX_RAW = "{\"account_number\":\"1\",\"chain_id\":\"tendermint_test\",\"fee\":{\"amount\":[{\"amount\":\"0\",\"denom\":\"\"}],\"gas\":\"21906\"},\"memo\":\"\",\"msgs\":[{\"type\":\"cosmos-sdk/MsgDelegate\",\"value\":{\"msg\":[{\"type\":\"cosmos-sdk/MsgDelegate\",\"value\":{\"delegation\":{\"amount\":\"100\",\"denom\":\"STAKE\"},\"delegator_address\":\"cosmos1zymku32dmnwwy0gwggxzzqqvzzfa2r0xthdlw0\",\"validator_address\":\"cosmosvaloper199mlc7fr6ll5t54w7tts7f4s0cvnqgc59nmuxf\"}}]}}],\"sequence\":\"0\"}";
    printf("\n %s:",TX_RAW);

    //get Cosmos as coin
    const CoinType *coin = fsm_getCoin(true, "Cosmos");
    if (!coin) return;

    //get curve
    const curve_info *curve = get_curve_by_name(coin->curve_name);

    //
    uint8_t hash[HASHER_DIGEST_LENGTH];
    uint8_t pby;
    uint8_t *signature;

    printf("\nhash:  ***************************");
    printf("\nhash:  %d:",hash);
    printf("\nhash:  ************************");

    //get hash
    //cryptoMessageHash(coin, curve, TX_RAW, message_len, hash)

    //sign hash
    Hasher hasher;
    hasher_Init(&hasher, curve->hasher_sign);
    //hasher_Update(&hasher, (const uint8_t *)coin->signed_message_header, strlen(coin->signed_message_header));
    uint8_t varint[5];
    uint32_t l = sizeof(TX_RAW);

    printf("\nlen :  ***************************");
    printf("\nlen :  %d:",l );
    printf("\nlen :  ************************");

    hasher_Update(&hasher, varint, l);
    hasher_Update(&hasher, TX_RAW, l);
    hasher_Final(&hasher, hash);
//

    printf("\nhash2:  ***************************");
    printf("\nhash2:  %d:",hash);
    printf("\nhash2:  ************************");

    //sign hash
    //int result = hdnode_sign_digest(node, hash, signature + 1, &pby, NULL);

    //printf("\nresult:  ***************************");
    //printf("\nresult:  %d:",result);
    //printf("\nresult:  ************************");

    /*
     *   END cosmos inject
     */


    /* Stage 1: Calculate total RLP length */
    uint32_t rlp_length = 0;
    layoutProgress(_("Signing"), 0);

    rlp_length += rlp_calculate_length(msg->nonce.size, msg->nonce.bytes[0]);
    rlp_length += rlp_calculate_length(msg->gas_price.size, msg->gas_price.bytes[0]);
    rlp_length += rlp_calculate_length(msg->gas_limit.size, msg->gas_limit.bytes[0]);
    rlp_length += rlp_calculate_length(msg->to.size, msg->to.bytes[0]);
    rlp_length += rlp_calculate_length(msg->value.size, msg->value.bytes[0]);
    rlp_length += rlp_calculate_length(data_total, msg->data_initial_chunk.bytes[0]);
    if (tx_type) {
        rlp_length += rlp_calculate_number_length(tx_type);
    }
    if (chain_id) {
        rlp_length += rlp_calculate_number_length(chain_id);
        rlp_length += rlp_calculate_length(0, 0);
        rlp_length += rlp_calculate_length(0, 0);
    }

    /* Stage 2: Store header fields */
    hash_rlp_list_length(rlp_length);
    layoutProgress(_("Signing"), 100);

    if (tx_type) {
        hash_rlp_number(tx_type);
    }
    hash_rlp_field(msg->nonce.bytes, msg->nonce.size);
    hash_rlp_field(msg->gas_price.bytes, msg->gas_price.size);
    hash_rlp_field(msg->gas_limit.bytes, msg->gas_limit.size);
    hash_rlp_field(msg->to.bytes, msg->to.size);
    hash_rlp_field(msg->value.bytes, msg->value.size);
    hash_rlp_length(data_total, msg->data_initial_chunk.bytes[0]);
    hash_data(msg->data_initial_chunk.bytes, msg->data_initial_chunk.size);
    data_left = data_total - msg->data_initial_chunk.size;

    memcpy(privkey, node->private_key, 32);

    if (data_left > 0) {
        send_request_chunk();
    } else {
        send_signature();
    }
}

//static void cryptoMessageHash(const CoinType *coin, const curve_info *curve, const uint8_t *message, size_t message_len, uint8_t hash[HASHER_DIGEST_LENGTH]) {
//    Hasher hasher;
//    hasher_Init(&hasher, curve->hasher_sign);
//    hasher_Update(&hasher, (const uint8_t *)coin->signed_message_header, strlen(coin->signed_message_header));
//    uint8_t varint[5];
//    uint32_t l = ser_length(message_len, varint);
//    hasher_Update(&hasher, varint, l);
//    hasher_Update(&hasher, message, message_len);
//    hasher_Final(&hasher, hash);
//}
//
//int cryptoMessageSign(const CoinType *coin, HDNode *node, InputScriptType script_type, const uint8_t *message, size_t message_len, uint8_t *signature)
//{
//    const curve_info *curve = get_curve_by_name(coin->curve_name);
//    if (!curve) return 1;
//    uint8_t hash[HASHER_DIGEST_LENGTH];
//    cryptoMessageHash(coin, curve, message, message_len, hash);
//
//    uint8_t pby;
//    int result = hdnode_sign_digest(node, hash, signature + 1, &pby, NULL);
//    if (result == 0) {
//        switch (script_type) {
//            case InputScriptType_SPENDP2SHWITNESS:
//                // segwit-in-p2sh
//                signature[0] = 35 + pby;
//                break;
//            case InputScriptType_SPENDWITNESS:
//                // segwit
//                signature[0] = 39 + pby;
//                break;
//            default:
//                // p2pkh
//                signature[0] = 31 + pby;
//                break;
//        }
//    }
//    return result;
//}


void cosmos_signing_txack(CosmosTxAck *tx)
{
    if (!cosmos_signing) {
        fsm_sendFailure(FailureType_Failure_UnexpectedMessage, _("Not in Cosmos signing mode"));
        layoutHome();
        return;
    }

    if (tx->data_chunk.size > data_left) {
        fsm_sendFailure(FailureType_Failure_Other, _("Too much data"));
        cosmos_signing_abort();
        return;
    }

    if (data_left > 0 && (!tx->has_data_chunk || tx->data_chunk.size == 0)) {
        fsm_sendFailure(FailureType_Failure_Other, _("Empty data chunk received"));
        cosmos_signing_abort();
        return;
    }

    hash_data(tx->data_chunk.bytes, tx->data_chunk.size);

    data_left -= tx->data_chunk.size;

    if (data_left > 0) {
        send_request_chunk();
    } else {
        send_signature();
    }
}

void cosmos_signing_abort(void)
{
    if (cosmos_signing) {
        memzero(privkey, sizeof(privkey));
        layoutHome();
        cosmos_signing = false;
    }
}

//static void cosmos_message_hash(const uint8_t *message, size_t message_len, uint8_t hash[32])
//{
//    struct SHA3_CTX ctx;
//    sha3_256_Init(&ctx);
//    sha3_Update(&ctx, (const uint8_t *)"\x19" "Cosmos Signed Message:\n", 26);
//    uint8_t c;
//    if (message_len >= 1000000000) { c = '0' + message_len / 1000000000 % 10; sha3_Update(&ctx, &c, 1); }
//    if (message_len >= 100000000)  { c = '0' + message_len / 100000000  % 10; sha3_Update(&ctx, &c, 1); }
//    if (message_len >= 10000000)   { c = '0' + message_len / 10000000   % 10; sha3_Update(&ctx, &c, 1); }
//    if (message_len >= 1000000)    { c = '0' + message_len / 1000000    % 10; sha3_Update(&ctx, &c, 1); }
//    if (message_len >= 100000)     { c = '0' + message_len / 100000     % 10; sha3_Update(&ctx, &c, 1); }
//    if (message_len >= 10000)      { c = '0' + message_len / 10000      % 10; sha3_Update(&ctx, &c, 1); }
//    if (message_len >= 1000)       { c = '0' + message_len / 1000       % 10; sha3_Update(&ctx, &c, 1); }
//    if (message_len >= 100)        { c = '0' + message_len / 100        % 10; sha3_Update(&ctx, &c, 1); }
//    if (message_len >= 10)         { c = '0' + message_len / 10         % 10; sha3_Update(&ctx, &c, 1); }
//    c = '0' + message_len              % 10; sha3_Update(&ctx, &c, 1);
//    sha3_Update(&ctx, message, message_len);
//    keccak_Final(&ctx, hash);
//}

//void cosmos_message_sign(const CosmosSignMessage *msg, const HDNode *node, CosmosMessageSignature *resp)
//{
//    uint8_t hash[32];
//
//    if (!hdnode_get_ethereum_pubkeyhash(node, resp->address.bytes)) {
//        return;
//    }
//    resp->has_address = true;
//    resp->address.size = 20;
//    cosmos_message_hash(msg->message.bytes, msg->message.size, hash);
//
//    uint8_t v;
//    if (ecdsa_sign_digest(&secp256k1, node->private_key, hash, resp->signature.bytes, &v, cosmos_is_canonic) != 0) {
//        fsm_sendFailure(FailureType_Failure_Other, _("Signing failed"));
//        return;
//    }
//
//    resp->has_signature = true;
//    resp->signature.bytes[64] = 27 + v;
//    resp->signature.size = 65;
//    msg_write(MessageType_MessageType_CosmosMessageSignature, resp);
//}

//int cosmos_message_verify(const CosmosVerifyMessage *msg)
//{
//    if (msg->signature.size != 65 || msg->address.size != 20) {
//        fsm_sendFailure(FailureType_Failure_SyntaxError, _("Malformed data"));
//        return 1;
//    }
//
//    uint8_t pubkey[65];
//    uint8_t hash[32];
//
//    cosmos_message_hash(msg->message.bytes, msg->message.size, hash);
//
//    /* v should be 27, 28 but some implementations use 0,1.  We are
//     * compatible with both.
//     */
//    uint8_t v = msg->signature.bytes[64];
//    if (v >= 27) {
//        v -= 27;
//    }
//    if (v >= 2 ||
//        ecdsa_recover_pub_from_sig(&secp256k1, pubkey, msg->signature.bytes, hash, v) != 0) {
//        return 2;
//    }
//
//    struct SHA3_CTX ctx;
//    sha3_256_Init(&ctx);
//    sha3_Update(&ctx, pubkey + 1, 64);
//    keccak_Final(&ctx, hash);
//
//    /* result are the least significant 160 bits */
//    if (memcmp(msg->address.bytes, hash + 12, 20) != 0) {
//        return 2;
//    }
//    return 0;
//}
