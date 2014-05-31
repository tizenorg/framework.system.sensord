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

#ifndef COMMAND_CHANNEL_H_
#define COMMAND_CHANNEL_H_
#include <sf_common.h>
#include <sensor.h>
#include <csock.h>
#include <cpacket.h>
#include <vector>

using std::vector;

class command_channel
{
public:

	command_channel();
	~command_channel();

	bool create_channel(void);
	bool cmd_get_id(int &client_id);
	bool cmd_hello(int client_id,  sensor_type sensor);
	bool cmd_byebye(int client_id, sensor_type sensor);
	bool cmd_start(int client_id, sensor_type sensor);
	bool cmd_stop(int client_id, sensor_type sensor);
	bool cmd_set_option(int client_id, sensor_type sensor, int option);
	bool cmd_register_event(int client_id, unsigned int event_type);
	bool cmd_register_events(int client_id, event_type_vector &event_vec);
	bool cmd_unregister_event(int client_id, unsigned int event_type);
	bool cmd_unregister_events(int client_id, event_type_vector &event_vec);
	bool cmd_check_event(int client_id, unsigned int event_type);
	bool cmd_set_interval(int client_id, sensor_type sensor, unsigned int interval);
	bool cmd_unset_interval(int client_id, sensor_type sensor);
	bool cmd_get_property(int client_id, sensor_type sensor, unsigned int data_id, void *property_data);
	bool cmd_set_value(int client_id, sensor_type sensor, unsigned int property, long value);
	bool cmd_get_struct(int client_id, unsigned int data_id, sensor_data_t* values);
	bool cmd_send_sensorhub_data(int client_id, int data_len, const char* buffer);
private:
	csock* m_command_sock;
	bool command_handler(cpacket *packet, void **return_payload);
};

#endif /* COMMAND_CHANNEL_H_ */
