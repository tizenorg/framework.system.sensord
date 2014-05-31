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

#ifndef CSENSOR_EVENT_DISPATCHER_CLASS_H_
#define CSENSOR_EVENT_DISPATCHER_CLASS_H_

#include <sf_common.h>
#include <csensor_event_queue.h>
#include <cclient_info_manager.h>
#include <cevent_socket.h>
#include <vconf.h>

typedef map<unsigned int, sensor_event_t> event_type_last_event_map;

class csensor_event_dispatcher
{
private:
	static csensor_event_dispatcher inst;
	bool m_lcd_on;
	cevent_socket m_accept_socket;
	cmutex m_mutex;
	cmutex m_last_events_mutex;
	event_type_last_event_map m_last_events;

	csensor_event_dispatcher();
	~csensor_event_dispatcher();
	csensor_event_dispatcher(csensor_event_dispatcher const&) {};
	csensor_event_dispatcher& operator=(csensor_event_dispatcher const&);

	void accept_connections(void);
	void accept_event_channel(cevent_socket client_socket);
	void dispatch_event(void);
	static void situation_watcher(keynode_t *node, void *user_data);
	bool is_lcd_on(void);
	static cclient_info_manager& get_client_info_manager(void);
	static csensor_event_queue& get_event_queue(void);

	bool is_record_event(unsigned int event_type);
	void put_last_event(unsigned int event_type, const sensor_event_t &event);
	bool get_last_event(unsigned int event_type, sensor_event_t &event);
public:
	static csensor_event_dispatcher& get_instance() {return inst;}
	bool run(void);
	void request_last_event(int client_id, const sensor_type sensor);
};

#endif
