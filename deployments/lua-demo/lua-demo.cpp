/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <iostream>
#include <string>
#include <cstring>

#include "service/lua/client/lua_client.h"
#include "protocols/service/lua/status.h"
#include "service_locator.h"


static void run_lua_test(struct lua_client *client, const char *script, const char *test_name)
{
	auto log = [test_name](const std::string &operation, int status, const std::string &msg = "")
	{
		std::ostream &os = (status == LUA_SUCCESS ? std::cout : std::cerr);
		os << "[" << test_name << "]" << operation << (status == LUA_SUCCESS ? " succeeded" : " failed") <<
			  " (status=" << status << ")";
		if (!msg.empty()) os << ": " << msg;
		os << std::endl;
	};

	int32_t env = -1;
	lua_status_t status = env_create(client, &env);
	log("env_create", status);

	status = env_append(client, env,
						reinterpret_cast<const uint8_t *>(script),
						std::strlen(script));
	log("env_append", status);

	uint8_t err_buf[512];
	size_t err_len = 0;
	status = env_execute(client, env, sizeof(err_buf), err_buf, &err_len);
	std::string msg(reinterpret_cast<char *>(err_buf), err_len);
	log("env_execute", status, msg);

	status = env_delete(client, env);
	log("env_delete", status);
}

int main()
{
	service_locator_init();

	/* Find and open RPC session to LUA SP */
	service_context *m_service_context = service_locator_query("sn:trustedfirmware.org:lua:0");
	if (!m_service_context) {
		std::cerr << "Failed to locate Lua service." << std::endl;
		return 1;
	}

	rpc_caller_session *m_rpc_session = service_context_open(m_service_context);
	if (!m_rpc_session) {
		std::cerr << "Failed to open RPC session to Lua service." << std::endl;
		service_context_relinquish(m_service_context);
		return 1;
	}

	struct lua_client m_lua_client;
	lua_client_init(&m_lua_client, m_rpc_session);

	/* Test 1: factorial */
	const char *test_script_1 = R"(
		function factorial(n)
			if n == 0 then
				return 1
			else
				return n * factorial(n - 1)
			end
		end
		print(("Factorial of %d is: %d"):format(5, factorial(5)))
	)";
	run_lua_test(&m_lua_client, test_script_1, "test_script_1");

	/* Test 2: sorting */
	const char *test_script_2 = R"(
		local numbers = {5, -1, 8, 3, -200, 999}
		table.sort(numbers)
		print("Sorted list:")
		for i, v in ipairs(numbers) do
			print(v)
		end
	)";
	run_lua_test(&m_lua_client, test_script_2, "test_script_2");

	/* Test 3: parser error */
	const char *test_script_3 = R"(
		function fn_with_syntax_error()
			print("Hello World!"
	)";
	run_lua_test(&m_lua_client, test_script_3, "test_script_3");

	/* Test 4: runtime error */
	const char *test_script_4 = R"(
		function raise_error()
			error("Something went wrong...")
		end
		raise_error()
	)";
	run_lua_test(&m_lua_client, test_script_4, "test_script_4");

	/* Cleanup */
	lua_client_deinit(&m_lua_client);
	service_context_close(m_service_context, m_rpc_session);
	service_context_relinquish(m_service_context);

	return 0;
}
