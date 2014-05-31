/*
 * sensor-framework
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef CSERVER_H_
#define CSERVER_H_

#include <cclient_info_manager.h>
#include <csensor_event_dispatcher.h>

class cserver
{
private:
	struct client_ctx_t {
		int	client_id;
		ctype *module;
		int client_state;
		unsigned int reg_state;
	};

	cmd_func_t m_cmd_handler[CMD_LAST];

	static cserver inst;

	cserver();

	static void *cb_ipc_worker(void *data);
	static void *cb_ipc_start(void *data);
	static void *cb_ipc_stop(void *data);

	void  command_handler(void *cmd_item, void *ctx);


	static void *cmd_get_id(void *cmd_item, void *data);
	static void *cmd_hello(void *cmd_item, void *data);
	static void *cmd_byebye(void *cmd_item, void *data);
	static void *cmd_get_value(void *cmd_item, void *data);
	static void *cmd_start(void *cmd_item, void *data);
	static void *cmd_stop(void *cmd_item, void *data);
	static void *cmd_register_event(void *cmd_item, void *data);
	static void *cmd_unregister_event(void *cmd_item, void *data);
	static void *cmd_check_event(void *cmd_item, void *data);
	static void *cmd_set_interval(void *cmd_item, void *data);
	static void *cmd_unset_interval(void *cmd_item, void *data);
	static void *cmd_set_option(void *cmd_item, void *data);
	static void *cmd_set_value(void *cmd_item, void *data);
	static void *cmd_get_property(void *cmd_item, void *data);
	static void *cmd_get_struct(void *cmd_item, void *data);
	static void *cmd_send_sensorhub_data(void *cmd_item, void *data);

	static cclient_info_manager& get_client_info_manager(void);
	static csensor_event_dispatcher& get_event_dispathcher(void);

	int get_systemd_socket(const char *name);
public:
	enum {
		MAX_CMD_COUNT = 256,
	};

	void sf_main_loop(void);
	void sf_main_loop_stop(void);
	static cserver& get_instance() {return inst;}
};

#endif
//! End of a file
