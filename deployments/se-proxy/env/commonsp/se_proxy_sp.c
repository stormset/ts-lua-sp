// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2025, Arm Limited and Contributors. All rights reserved.
 */

#include "components/rpc/common/endpoint/rpc_service_interface.h"
#include "components/rpc/ts_rpc/endpoint/sp/ts_rpc_endpoint_sp.h"
#include "config/ramstore/config_ramstore.h"
#include "config/loader/sp/sp_config_loader.h"
#include "sp_api.h"
#include "sp_discovery.h"
#include "sp_rxtx.h"
#include "trace.h"
#include "deployments/se-proxy/env/commonsp/proxy_service_factory_list.h"

#include <stddef.h>

/*
 * This must be a global variable so the communication layer (e.g. RSE_COMMS)
 * can use this to remap message client_ids if needed.
 */
uint16_t own_id;

static bool sp_init(uint16_t *own_sp_id);

void __noreturn sp_main(union ffa_boot_info *boot_info)
{
	struct ts_rpc_endpoint_sp rpc_endpoint = { 0 };
	struct sp_msg req_msg = { 0 };
	struct sp_msg resp_msg = { 0 };
	struct rpc_service_interface *rpc_iface = NULL;
	sp_result result = SP_RESULT_INTERNAL_ERROR;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	unsigned int n_services = PROXY_SERVICE_FACTORY_LIST_LENGTH();
	/* Two memory shares for each service, plus additional 6 extra slots. */
	unsigned int n_shares = (n_services * 2) + 6;
	const struct se_proxy_list_entry *service_factory_iterator = __proxy_service_factory_list;

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

	rpc_status = ts_rpc_endpoint_sp_init(&rpc_endpoint, n_services, n_shares);
	if (rpc_status != RPC_SUCCESS) {
		EMSG("Failed to initialize RPC endpoint: %d", rpc_status);
		goto fatal_error;
	}
	IMSG("Created RPC endpoint for %u service and %u memory shares.", n_services, n_shares);

	/* Create services */
	if (service_factory_iterator == __proxy_service_factory_list_end) {
		EMSG("No services to construct.");
		goto fatal_error;
	}

	/* Iterate the proxy_service_factory_list created by the linker.
	 * See: proxy_service_factory_list.h
	 */
	while (service_factory_iterator < __proxy_service_factory_list_end) {

		IMSG("Creating proxy service %s", service_factory_iterator->name);

		rpc_iface = service_factory_iterator->fn();
		if (!rpc_iface) {
			EMSG("Failed to create service %s", service_factory_iterator->name);
			goto fatal_error;
		}
		rpc_status = ts_rpc_endpoint_sp_add_service(&rpc_endpoint, rpc_iface);

		if (rpc_status != RPC_SUCCESS) {
			EMSG("Failed to add service to RPC endpoint: %d", rpc_status);
			goto fatal_error;
		}
		service_factory_iterator ++;
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
	EMSG("SE proxy SP error");
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

	sp_res = sp_discovery_own_id_get(own_id);
	if (sp_res != SP_RESULT_OK) {
		EMSG("Failed to query own ID: %d", sp_res);
		return false;
	}

	return true;
}
