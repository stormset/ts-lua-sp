#-------------------------------------------------------------------------------
# Copyright (c) 2022-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# A stub infrastructure for the se-proxy. Infrastructure dependencies are all
# realized with stub components that do absolutely nothing.
#-------------------------------------------------------------------------------

# Configure supported set of proxy services.
set(CFG_ENABLE_ITS           On)
set(CFG_ENABLE_PS            On)
set(CFG_ENABLE_CRYPTO        On)
set(CFG_ENABLE_CRYPTO_NANO   On)
set(CFG_ENABLE_IAT           On)
set(CFG_ENABLE_FWU           On)
#-------------------------------------------------------------------------------
# Infrastructure components
#
#-------------------------------------------------------------------------------
add_components(TARGET "se-proxy"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/uuid"
		"components/rpc/dummy"
		"components/rpc/common/caller"
		"components/service/attestation/reporter/stub"
		"components/service/attestation/key_mngr/stub"
		"components/service/crypto/backend/stub"
		"components/service/crypto/client/psa"
		"components/service/fwu/common"
		"components/service/fwu/provider"
		"components/service/fwu/provider/serializer"
		"components/service/fwu/psa_fwu_m/agent"
		"components/service/fwu/psa_fwu_m/interface/stub"
		"components/service/secure_storage/backend/mock_store"
)

target_sources(se-proxy PRIVATE

	${CMAKE_CURRENT_LIST_DIR}/service_proxy_factory.c
)
