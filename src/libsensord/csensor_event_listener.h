/*
 * libslp-sensor
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

#ifndef CSENSOR_EVENT_LISTENER_H_
#define CSENSOR_EVENT_LISTENER_H_

#include <glib.h>
#include <sys/types.h>
#include <csensor_handle_info.h>
#include <unistd.h>
#include <cevent_socket.h>
#include <string.h>
#include <sf_common.h>
#include <algorithm>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cmutex.h>
#include <poller.h>

using std::map;
using std::vector;
using std::string;
using std::queue;
using std::mutex;
using std::lock_guard;
using std::unique_lock;
using std::condition_variable;

typedef vector<unsigned int> handle_vector;
typedef vector<sensor_type> sensor_type_vector;
typedef map<int,vector<unsigned int>> handle_events_map;
typedef map<int,csensor_handle_info> sensor_handle_info_map;
typedef map<sensor_type, command_channel*> sensor_command_channel_map;

typedef struct {
	sensor_callback_func_t callback;
	unsigned int event_type;
	sensor_event_data_t event_data;
	void *data;
} client_callback_info;

typedef struct sensor_rep
{
	bool active;
	int option;
	unsigned int interval;
	event_type_vector event_types;
} sensor_rep;

class csensor_event_listener {
public:
	csensor_event_listener();
	~csensor_event_listener();

	int create_handle(const sensor_type sensor);
	bool delete_handle(const int handle);
	bool start_handle(const int handle);
	bool stop_handle(const int handle);
	bool register_event(const int handle, const unsigned int event_type, const unsigned int interval,
			const sensor_callback_func_t callback, void* cb_data);
	bool unregister_event(const int handle, const unsigned int event_type);
	bool change_event_interval(const int handle, const unsigned int event_type, const unsigned int interval);

	bool set_sensor_params(const int handle, int sensor_state, int sensor_option);
	bool set_sensor_state(const int handle, const int sensor_state);
	bool set_sensor_option(const int handle, const int sensor_option);
	bool set_event_interval(const int handle, const unsigned int event_type, const unsigned int interval);

	void get_listening_sensors(sensor_type_vector &sensors);

	void pause_sensor(const sensor_type sensor);
	void resume_sensor(const sensor_type sensor);

	unsigned int get_active_min_interval(const sensor_type sensor);
	unsigned int get_active_option(const sensor_type sensor);
	void get_active_event_types(const sensor_type sensor, event_type_vector &active_event_types);

	bool get_sensor_type(const int handle, sensor_type &sensor);
	bool get_sensor_state(const int handle, int &state);

	void get_sensor_rep(sensor_type sensor, sensor_rep& rep);

	bool has_client_id(void);
	int get_client_id(void);
	void set_client_id(const int client_id);
	int check_event_registered(const sensor_type sensor, const unsigned int event_type);

	bool is_active(void);
	bool is_sensor_registered(const sensor_type sensor);
	bool is_sensor_active(const sensor_type sensor);

	bool set_command_channel(sensor_type sensor, command_channel *cmd_channel);
	bool get_command_channel(sensor_type sensor, command_channel **cmd_channel);
	bool close_command_channel(sensor_type sensor);

	void get_all_handles(handle_vector &handles);

	int get_single_event_count(const unsigned int event_type);
	bool start_event_listener(void);
	void stop_event_listener(void);

private:
	enum thread_state {
		THREAD_STATE_START,
		THREAD_STATE_STOP,
		THREAD_STATE_TERMINATE,
	};
	typedef lock_guard<mutex> lock;
	typedef unique_lock<mutex> ulock;

	sensor_handle_info_map m_sensor_handle_infos;
	sensor_command_channel_map m_command_channels;

	int m_client_id;

	cevent_socket m_event_socket;
	poller *m_poller;

	cmutex m_handle_info_lock;

	thread_state m_thread_state;
	mutex m_thread_mutex;
	condition_variable m_thread_cond;

	bool create_event_channel(void);
	void close_event_channel(void);

	bool sensor_event_poll(void* buffer, int buffer_len);

	void listen_events(void);
	void handle_events(void* event);

	client_callback_info* get_callback_info(creg_event_info &event_info, sensor_event_data_t &event_data);

	void post_callback_to_main_loop(client_callback_info *cb_info);
	static gboolean callback_dispatcher(gpointer data);

	void set_thread_state(thread_state state);
};
#endif /* CSENSOR_EVENT_LISTENER_H_ */
