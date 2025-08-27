# Lua SP for Trusted Services

## Overview

This project adds **Lua SP** to Trusted Services: a Lua interpreter deployed inside an SP environment.
It allows scripts supplied from *Normal World* to be executed in an isolated SP environment
(mainly for testing purposes).

Currently only C binding to the SPs logging infrastructure is established.
With additional bindings to libsp and the PSA APIs it will allow running tests coordinated from Normal World without precompiling them into the SP binary.

## Future pointers

* To avoid hand-writing C-bindings for all API functions and maintaining these as project grows FFI solutions should be evaluated (e.g. https://github.com/mascarenhas/alien). This could be achieved by implementing bindings to LUA's file system related functions that can access semi-hosted host OS file system through RPC.



## Running *lua-demo*

A simple application was created to verify the SPs behaviour. In order to run it follow these steps:

1. Set-up your build environment according to the [instructions here](https://trusted-services.readthedocs.io/en/integration/developer/build-instructions.html)

2. Apply (`git apply`) the following patch to `fvp-psa-sp.mk` (inside `build` folder after repo sync):

```
diff --git a/fvp-psa-sp.mk b/fvp-psa-sp.mk
index 2a8968b..1a4e70a 100644
--- a/fvp-psa-sp.mk
+++ b/fvp-psa-sp.mk
@@ -7,6 +7,7 @@ TS_LOGGING_SP			?= y
 TS_LOGGING_SP_LOG		?= "trusted-services-logs.txt"
 TS_UEFI_TESTS			?= n
 TS_FW_UPDATE			?= n
+TS_LUA_SP				?= y
 TS_UEFI_AUTH_VAR 		?= y
 TS_UEFI_INTERNAL_CRYPTO	?= n
 # Supported values: embedded, fip
@@ -66,6 +67,7 @@ SP_PSA_ATTESTATION_CONFIG	?= $(DEFAULT_SP_CONFIG)
 SP_SMM_GATEWAY_CONFIG		?= $(DEFAULT_SP_CONFIG)
 SP_FWU_CONFIG			?= $(DEFAULT_SP_CONFIG)
 SP_LOGGING_CONFIG		?= $(DEFAULT_SP_CONFIG)
+SP_LUA_CONFIG			?= $(DEFAULT_SP_CONFIG)
 
 LINUX_DEFCONFIG_COMMON_FILES ?= $(CURDIR)/kconfigs/fvp_trusted-services.conf
 
@@ -142,6 +144,9 @@ endif
 ifeq ($(TS_FW_UPDATE),y)
 $(eval $(call build-sp,fwu,config/$(SP_FWU_CONFIG),6823a838-1b06-470e-9774-0cce8bfb53fd,$(SP_FWU_EXTRA_FLAGS)))
 endif
+ifeq ($(TS_LUA_SP),y)
+$(eval $(call build-sp,lua,config/$(SP_LUA_CONFIG),cf0cfcf8-8376-46ad-903f-777eceb8af2a,$(SP_LUA_EXTRA_FLAGS)))
+endif
 else
 # SPMC test SPs
 OPTEE_OS_COMMON_EXTRA_FLAGS	+= CFG_SPMC_TESTS=y
```

3. Build `lua-demo`:
```
pushd trusted-services/deployments/lua-demo/arm-linux && (mkdir build && cd build && cmake .. && make) && popd
```


4. Run emulator and once Linux is booted do:
```
cd /mnt/host
LD_PRELOAD=out/ts-install/arm-linux/lib/libtsd.so trusted-services/deployments/lua-demo/arm-linux/build/lua-demo
```

You will be able to see logs originating from `print()` calls within your script on the console bound to the logging SP.\
You will also be able to follow what service calls are being executed on Linux's console.