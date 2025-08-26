// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
 */

#include "components/rpc/common/endpoint/rpc_service_interface.h"
#include "components/rpc/ts_rpc/endpoint/sp/ts_rpc_endpoint_sp.h"
#include "service/secure_storage/factory/storage_factory.h"
#include "service/crypto/factory/crypto_provider_factory.h"
#include "service/crypto/backend/mbedcrypto/mbedcrypto_backend.h"
#include "service/log/factory/log_factory.h"
#include "protocols/rpc/common/packed-c/status.h"
#include "config/ramstore/config_ramstore.h"
#include "config/loader/sp/sp_config_loader.h"
#include "sp_api.h"
#include "sp_discovery.h"
#include "sp_messaging.h"
#include "sp_rxtx.h"
#include "trace.h"

static bool sp_init(uint16_t *own_sp_id);

void __noreturn sp_main(union ffa_boot_info *boot_info)
{
	struct crypto_provider *crypto_provider = NULL;
	struct crypto_provider *crypto_protobuf_provider = NULL;
	struct ts_rpc_endpoint_sp rpc_endpoint = { 0 };
	struct rpc_service_interface *crypto_iface = NULL;
	struct rpc_service_interface *crypto_iface_protobuf = NULL;
	struct sp_msg req_msg = { 0 };
	struct sp_msg resp_msg = { 0 };
	struct storage_backend *storage_backend = NULL;
	uint16_t own_id = 0;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	sp_result result = SP_RESULT_INTERNAL_ERROR;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;

	/* Boot phase */
	if (!sp_init(&own_id)) {
		EMSG("Failed to init SP");
		goto fatal_error;
	}

	config_ramstore_init();

	if (!sp_config_load(boot_info)) {
		EMSG("Failed to load SP config");
		goto fatal_error;
	}

	/* Create a storage backend for persistent key storage - prefer ITS */
	storage_backend = storage_factory_create(storage_factory_security_class_INTERNAL_TRUSTED);
	if (!storage_backend) {
		EMSG("Failed to create storage factory");
		goto fatal_error;
	}

	/* Initialize the crypto service */
	psa_status = mbedcrypto_backend_init(storage_backend, 0);
	if (psa_status != PSA_SUCCESS) {
		EMSG("Failed to init Mbed TLS backend: %d", psa_status);
		goto fatal_error;
	}

	crypto_provider = crypto_provider_factory_create();
	if (!crypto_provider) {
		EMSG("Failed to create crypto provider factory");
		goto fatal_error;
	}

	crypto_protobuf_provider = crypto_protobuf_provider_factory_create();
	if (!crypto_protobuf_provider) {
		EMSG("Failed to create crypto protobuf provider factory");
		goto fatal_error;
	}

	crypto_iface = service_provider_get_rpc_interface(&crypto_provider->base_provider);
	if (!crypto_iface) {
		EMSG("Failed to create service provider RPC interface");
		goto fatal_error;
	}

	crypto_iface_protobuf = service_provider_get_rpc_interface(
		&crypto_protobuf_provider->base_provider);
	if (!crypto_iface_protobuf) {
		EMSG("Failed to create service provider RPC interface");
		goto fatal_error;
	}

	rpc_status = ts_rpc_endpoint_sp_init(&rpc_endpoint, 2, 16);
	if (rpc_status != RPC_SUCCESS) {
		EMSG("Failed to initialize RPC endpoint: %d", rpc_status);
		goto fatal_error;
	}

	rpc_status = ts_rpc_endpoint_sp_add_service(&rpc_endpoint, crypto_iface);
	if (rpc_status != RPC_SUCCESS) {
		EMSG("Failed to add service to RPC endpoint: %d", rpc_status);
		goto fatal_error;
	}

	rpc_status = ts_rpc_endpoint_sp_add_service(&rpc_endpoint, crypto_iface_protobuf);
	if (rpc_status != RPC_SUCCESS) {
		EMSG("Failed to add service to RPC endpoint: %d", rpc_status);
		goto fatal_error;
	}

	/* End of boot phase */
	result = sp_msg_wait(&req_msg);
	if (result != SP_RESULT_OK) {
		EMSG("Failed to send message wait %d", result);
		goto fatal_error;
	}

	while (1) {
		ts_rpc_endpoint_sp_receive(&rpc_endpoint, &req_msg, &resp_msg);

		result = sp_msg_send_direct_resp(&resp_msg, &req_msg);
		if (result != SP_RESULT_OK) {
			EMSG("Failed to send direct response %d", result);
			result = sp_msg_wait(&req_msg);
			if (result != SP_RESULT_OK) {
				EMSG("Failed to send message wait %d", result);
				goto fatal_error;
			}
		}
	}

fatal_error:
	/* SP is not viable */
	EMSG("Crypto SP error");
	while (1) {}
}

void sp_interrupt_handler(uint32_t interrupt_id)
{
	(void)interrupt_id;
}

ffa_result ffa_vm_created_handler(uint16_t vm_id, uint64_t handle)
{
	(void)vm_id;
	(void)handle;

	return FFA_OK;
}

ffa_result ffa_vm_destroyed_handler(uint16_t vm_id, uint64_t handle)
{
	(void)vm_id;
	(void)handle;

	return FFA_OK;
}

static bool sp_init(uint16_t *own_id)
{
	sp_result sp_res = SP_RESULT_INTERNAL_ERROR;
	static uint8_t tx_buffer[4096] __aligned(4096);
	static uint8_t rx_buffer[4096] __aligned(4096);

	sp_res = sp_rxtx_buffer_map(tx_buffer, rx_buffer, sizeof(rx_buffer));
	if (sp_res != SP_RESULT_OK) {
		EMSG("Failed to map RXTX buffers: %d", sp_res);
		return false;
	}

	IMSG("Start discovering logging service");
	if (log_factory_create()) {
		IMSG("Logging service discovery successful");
	} else {
		EMSG("Logging service discovery failed, falling back to console log");
	}

	sp_res = sp_discovery_own_id_get(own_id);
	if (sp_res != SP_RESULT_OK) {
		EMSG("Failed to query own ID: %d", sp_res);
		return false;
	}

	return true;
}
