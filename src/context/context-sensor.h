/*
 * sf-plugin-common
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

#if !defined(_CONTEXT_SENSOR_CLASS_H_)
#define _CONTEXT_SENSOR_CLASS_H_

#include <sensor.h>
#include <string>

using std::string;

class context_sensor:public csensor_module {
public:
	context_sensor();
	virtual ~ context_sensor();

	const char *name(void);
	int version(void);
	int id(void);

	bool is_data_ready(bool wait);

	bool update_name(char *name);
	bool update_version(int ver);
	bool update_id(int id);

	bool update_polling_interval(unsigned long val);
	int get_sensor_type(void);
	long set_cmd(int type, int property, long input_value);
	int get_property(unsigned int property_level, void *property_data);
	int get_struct_value(unsigned int struct_type, void *struct_values);
	int send_sensorhub_data(const char* data, int data_len);

	bool start(void);
	bool stop(void);
private:
	bool update_value(void);

	int open_input_node(const char* input_node);

	int print_context_data(const char* name, const char *data, int length);
	int send_context_data(const char *data, int data_len);
	int read_context_data(void);
	int read_large_context_data(void);

	int m_client;
	int m_sensor_type;
	string m_name;

	long m_id;
	long m_version;

	bool m_enabled;

	int m_node_handle;
	int m_context_fd;

	sensorhub_event_t m_pending_event;
	sensorhub_event_t m_event;

	cmutex m_mutex;
	cmutex m_event_mutex;

};

#endif
