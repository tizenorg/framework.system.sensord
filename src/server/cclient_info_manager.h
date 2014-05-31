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

#ifndef CCLIENT_INFO_MANAGER_H_
#define CCLIENT_INFO_MANAGER_H_

#include <cclient_sensor_record.h>
#include <map>
#include <common.h>
#include <cmutex.h>
using std::map;

typedef map<int,cclient_sensor_record> client_id_sensor_record_map;
typedef vector<int> client_id_vec;


class cclient_info_manager {
public:
	static cclient_info_manager& get_instance() {return inst;}

	int create_client_record(void);
	bool remove_client_record(const int client_id);
	bool has_client_record(int client_id);

	void set_client_info(int client_id, pid_t pid);
	const char* get_client_info(int client_id);

	bool create_sensor_record(int client_id, const sensor_type sensor);
	bool remove_sensor_record(const int client_id, const sensor_type sensor);
	bool has_sensor_record(const int client_id, const sensor_type sensor);
	bool has_sensor_record(const int client_id);

	bool register_event(const int client_id, const unsigned int event_type);
	bool unregister_event(const int client_id, const unsigned int event_type);
	bool is_sensor_event_registered(const int client_id, const unsigned int event_type);

	bool set_interval(const int client_id, const sensor_type sensor, const unsigned int interval);
	unsigned int get_interval(const int client_id, const sensor_type sensor);
	bool set_option(const int client_id, const sensor_type sensor, const int option);


	bool is_sensor_used(const sensor_type sensor, const event_situation mode);
	bool get_registered_events(const int client_id, const sensor_type sensor, event_type_vector &event_vec);

	bool get_listener_ids(const unsigned int event_type, const event_situation mode, client_id_vec &id_vec);
	bool get_event_socket(const int client_id, cevent_socket &sock);
	bool set_event_socket(const int client_id, cevent_socket sock);
private:
	static cclient_info_manager inst;
	client_id_sensor_record_map m_clients;
	cmutex m_mutex;

	cclient_info_manager();
	~cclient_info_manager();
	cclient_info_manager(cclient_info_manager const&) {};
	cclient_info_manager& operator=(cclient_info_manager const&);
};

#endif /* CCLIENT_INFO_MANAGER_H_ */
