// Copyright 2019 Shift Cryptosecurity AG
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "commander_btc.h"
#include "commander_states.h"

#include <stdio.h>

#include <apps/btc/btc.h>
#include <apps/btc/btc_sign.h>

static commander_error_t _result(app_btc_result_t result)
{
    switch (result) {
    case APP_BTC_OK:
        return COMMANDER_OK;
    case APP_BTC_ERR_USER_ABORT:
        return COMMANDER_ERR_USER_ABORT;
    case APP_BTC_ERR_INVALID_INPUT:
        return COMMANDER_ERR_INVALID_INPUT;
    case APP_BTC_ERR_DUPLICATE:
        return COMMANDER_ERR_DUPLICATE;
    case APP_BTC_ERR_STATE:
        return COMMANDER_ERR_INVALID_STATE;
    default:
        return COMMANDER_ERR_GENERIC;
    }
}

static commander_error_t _btc_pub_address_multisig(
    const BTCPubRequest* request,
    PubResponse* response)
{
    const BTCScriptConfig_Multisig* multisig = &request->output.script_config.config.multisig;
    app_btc_result_t result = app_btc_address_multisig(
        request->coin,
        multisig,
        request->keypath,
        request->keypath_count,
        response->pub,
        sizeof(response->pub),
        request->display);
    return _result(result);
}

commander_error_t commander_btc_pub(const BTCPubRequest* request, PubResponse* response)
{
    if (!app_btc_enabled(request->coin)) {
        return COMMANDER_ERR_DISABLED;
    }
    switch (request->which_output) {
    case BTCPubRequest_xpub_type_tag:
        // Handled in Rust.
        return COMMANDER_ERR_INVALID_INPUT;
    case BTCPubRequest_script_config_tag:
        switch (request->output.script_config.which_config) {
        case BTCScriptConfig_simple_type_tag:
            // Handled in Rust.
            return COMMANDER_ERR_INVALID_INPUT;
        case BTCScriptConfig_multisig_tag:
            return _btc_pub_address_multisig(request, response);
        default:
            return COMMANDER_ERR_INVALID_INPUT;
        }
    default:
        return COMMANDER_ERR_INVALID_INPUT;
    }
}

static commander_error_t _api_is_script_config_registered(
    const BTCIsScriptConfigRegisteredRequest* request,
    BTCIsScriptConfigRegisteredResponse* response)
{
    const BTCScriptConfigRegistration* reg = &request->registration;
    if (!app_btc_is_script_config_registered(
            reg->coin,
            &reg->script_config,
            reg->keypath,
            reg->keypath_count,
            &response->is_registered)) {
        return COMMANDER_ERR_GENERIC;
    }
    return COMMANDER_OK;
}

static commander_error_t _api_register_script_config(const BTCRegisterScriptConfigRequest* request)
{
    app_btc_result_t result = app_btc_register_script_config(
        request->registration.coin,
        &request->registration.script_config,
        request->registration.keypath,
        request->registration.keypath_count,
        request->name,
        request->xpub_type);
    return _result(result);
}

commander_error_t commander_btc(const BTCRequest* request, BTCResponse* response)
{
    switch (request->which_request) {
    case BTCRequest_is_script_config_registered_tag:
        response->which_response = BTCResponse_is_script_config_registered_tag;
        return _api_is_script_config_registered(
            &(request->request.is_script_config_registered),
            &response->response.is_script_config_registered);
    case BTCRequest_register_script_config_tag:
        response->which_response = BTCResponse_success_tag;
        return _api_register_script_config(&(request->request.register_script_config));
    default:
        return COMMANDER_ERR_GENERIC;
    }
    return COMMANDER_OK;
}
