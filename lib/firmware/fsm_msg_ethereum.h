static int process_ethereum_xfer(const CoinType *coin, EthereumSignTx *msg)
{
    int ret_val = TXOUT_COMPILE_ERROR;
    char node_str[NODE_STRING_LENGTH], amount_str[32], token_amount_str[128+sizeof(msg->token_shortcut)+2];
    const HDNode *node = NULL;

    /* Precheck: For TRANSFER, 'to' fields should not be loaded */
    if(msg->has_to || msg->to.size || strlen((char *)msg->to.bytes) != 0)
    {
        /* Bailing, error detected! */
        goto process_ethereum_xfer_exit;
    }

    if(bip32_node_to_string(node_str, sizeof(node_str), coin, msg->to_address_n, msg->to_address_n_count, /*whole_account=*/false))
    {
        ButtonRequestType button_request = ButtonRequestType_ButtonRequest_ConfirmTransferToAccount;
        if(is_token_transaction(msg)) {
            uint32_t decimal = ethereum_get_decimal(msg->token_shortcut);
            if (decimal == 0) {
                goto process_ethereum_xfer_exit;
            }

            if(ether_token_for_display(msg->token_value.bytes, msg->token_value.size, decimal, token_amount_str, sizeof(token_amount_str)))
            {
                // append token shortcut
                strncat(token_amount_str, " ", 1);
                strncat(token_amount_str, msg->token_shortcut, sizeof(msg->token_shortcut));
                if (!confirm_transfer_output(button_request, token_amount_str, node_str))
                {
                       ret_val = TXOUT_CANCEL;
                    goto process_ethereum_xfer_exit;
                }
            }
            else
            {
                    goto process_ethereum_xfer_exit;
            }
        }
        else
        {
                   if(ether_for_display(msg->value.bytes, msg->value.size, amount_str))
            {
                if(!confirm_transfer_output(button_request, amount_str, node_str))
                {
                    ret_val = TXOUT_CANCEL;
                    goto process_ethereum_xfer_exit;
                }
            }
            else
            {
                    goto process_ethereum_xfer_exit;
            }
        }
    }

    node = fsm_getDerivedNode(SECP256K1_NAME, msg->to_address_n, msg->to_address_n_count, NULL);
    if(node)
    {
        // setup "token_to" or "to" field depending on if this is a token transaction or not
        if (is_token_transaction(msg)) {
            if(hdnode_get_ethereum_pubkeyhash(node, msg->token_to.bytes))
            {
                msg->has_token_to = true;
                msg->token_to.size = 20;
                ret_val = TXOUT_OK;
               }
        }
        else
        {
            if(hdnode_get_ethereum_pubkeyhash(node, msg->to.bytes))
            {
                msg->has_to = true;
                msg->to.size = 20;
                ret_val = TXOUT_OK;
            }
        }
        memset((void *)node, 0, sizeof(HDNode));
    }

process_ethereum_xfer_exit:
    return(ret_val);
}

static int process_ethereum_msg(EthereumSignTx *msg, bool *confirm_ptr)
{
    int ret_result = TXOUT_COMPILE_ERROR;
    const CoinType *coin = fsm_getCoin(true, ETHEREUM);

    if(coin != NULL)
    {
        switch(msg->address_type)
        {
            case OutputAddressType_EXCHANGE:
            {
                /*prep for exchange type transaction*/
                HDNode *root_node = fsm_getDerivedNode(SECP256K1_NAME, 0, 0, NULL); /* root node */
                ret_result = run_policy_compile_output(coin, root_node, (void *)msg, (void *)NULL, true);
                if(ret_result < TXOUT_OK) {
                    memset((void *)root_node, 0, sizeof(HDNode));
                }
                *confirm_ptr = false;
                break;
            }
            case OutputAddressType_TRANSFER:
            {
                /*prep transfer type transaction*/
                ret_result = process_ethereum_xfer(coin, msg);
                *confirm_ptr = false;
                break;
            }
            default:
                ret_result = TXOUT_OK;
                break;
        }
    }
    return(ret_result);
}

void fsm_msgEthereumSignTx(EthereumSignTx *msg)
{
	CHECK_INITIALIZED

	CHECK_PIN_TXSIGN

	bool needs_confirm = true;
	int msg_result = process_ethereum_msg(msg, &needs_confirm);

	if (msg_result < TXOUT_OK) {
		send_fsm_co_error_message(msg_result);
		layoutHome();
		return;
	}

	const HDNode *node = fsm_getDerivedNode(SECP256K1_NAME, msg->address_n, msg->address_n_count, NULL);
	if (!node) return;

	ethereum_signing_init(msg, node, needs_confirm);
}

void fsm_msgEthereumTxAck(EthereumTxAck *msg)
{
	ethereum_signing_txack(msg);
}

void fsm_msgEthereumGetAddress(EthereumGetAddress *msg)
{
	RESP_INIT(EthereumAddress);

	CHECK_INITIALIZED

	CHECK_PIN

	const HDNode *node = fsm_getDerivedNode(SECP256K1_NAME, msg->address_n, msg->address_n_count, NULL);
	if (!node) return;

	resp->address.size = 20;

	if (!hdnode_get_ethereum_pubkeyhash(node, resp->address.bytes))
		return;

	if (msg->has_show_display && msg->show_display) {
		char address[43];
		format_ethereum_address(resp->address.bytes, address, sizeof(address));

		if (!confirm_ethereum_address("", address)) {
			fsm_sendFailure(FailureType_Failure_ActionCancelled, "Show address cancelled");
			layoutHome();
			return;
		}
	}

	msg_write(MessageType_MessageType_EthereumAddress, resp);
	layoutHome();
}

void fsm_msgEthereumSignMessage(EthereumSignMessage *msg)
{
	RESP_INIT(EthereumMessageSignature);

	CHECK_INITIALIZED

	if (!confirm(ButtonRequestType_ButtonRequest_ProtectCall, "Sign Message",
	             "%s", msg->message.bytes)) {
		fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
		layoutHome();
		return;
	}

	CHECK_PIN

	const HDNode *node = fsm_getDerivedNode(SECP256K1_NAME, msg->address_n, msg->address_n_count, NULL);
	if (!node) return;

	ethereum_message_sign(msg, node, resp);
	layoutHome();
}

void fsm_msgEthereumVerifyMessage(const EthereumVerifyMessage *msg)
{
	CHECK_PARAM(msg->has_address, _("No address provided"));
	CHECK_PARAM(msg->has_message, _("No message provided"));

	if (ethereum_message_verify(msg) != 0) {
		fsm_sendFailure(FailureType_Failure_SyntaxError, _("Invalid signature"));
		return;
	}

	char address[43] = { '0', 'x' };
	ethereum_address_checksum(msg->address.bytes, address + 2, false, 0);
#if 0
	// FIXME: verify address
	layoutVerifyAddress(address);
	if (!protectButton(ButtonRequestType_ButtonRequest_Other, false)) {
		fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
		layoutHome();
		return;
	}
#endif
	if (!confirm(ButtonRequestType_ButtonRequest_Other, "Message Verified", "%s",
	             msg->message.bytes)) {
		fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
		layoutHome();
		return;
	}
	fsm_sendSuccess(_("Message verified"));

	layoutHome();
}
