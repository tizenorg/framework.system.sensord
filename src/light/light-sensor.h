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

#if !defined(_LIGHT_SENSOR_CLASS_H_)
#define _LIGHT_SENSOR_CLASS_H_

#include <sensor.h>
#include <string>

using std::string;

class light_sensor:public csensor_module {
 public:
	 light_sensor();
	 virtual ~ light_sensor();

	const char *name(void);
	int version(void);
	int id(void);

	bool is_data_ready(bool wait = false);

	bool update_name(char *name);
	bool update_version(int ver);
	bool update_id(int id);

	bool update_polling_interval(unsigned long val);
	int get_sensor_type(void);
	bool check_hw_node(void);

	bool start(void);
	bool stop(void);

	long set_cmd(int type, int property, long input_value);

	int get_property(unsigned int property_level, void *property_data);
	int get_struct_value(unsigned int struct_type, void *struct_values);

 private:

	static const char *m_port[];

	string m_model_id;
	string m_name;
	string m_vendor;
	string m_chip_name;

	long m_id;
	long m_version;
	unsigned long m_polling_interval;

	int m_adc;
	int m_level;

	unsigned long long m_fired_time;

	int m_client;
	int m_node_handle;
	int m_sensor_type;

	string m_enable_resource;
	string m_resource;
	string m_polling_resource;

	cmutex m_value_mutex;

	bool update_value(bool wait = false);
	bool get_config(const string& element, const string& attr, string& value);
	bool get_config(const string& element, const string& attr, double& value);
	bool get_config(const string& element, const string& attr, long& value);
};

#endif
