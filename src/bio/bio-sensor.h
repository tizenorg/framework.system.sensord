/*
 * sf-plugin-common
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
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

#if !defined(_BIO_SENSOR_CLASS_H_)
#define _BIO_SENSOR_CLASS_H_

#include <sensor.h>
#include <string>

using std::string;

class bio_sensor : public csensor_module
{
public:
	bio_sensor();
	virtual ~bio_sensor();

	const char *name(void);
	int version(void);
	int id(void);

	bool is_data_ready(bool wait=false);

	bool update_name(char *name);
	bool update_version(int ver);
	bool update_id(int id);

	bool update_polling_interval(unsigned long val);
	int get_sensor_type(void);
	long set_cmd(int type , int property , long input_value);
	int get_property(unsigned int property_level , void *property_data);
	int get_struct_value(unsigned int struct_type , void *struct_values);


	bool check_hw_node(void);

	bool start(void);
	bool stop(void);

private:
	string m_model_id;
	string m_name;
	string m_vendor;
	string m_chip_name;

	string m_resource;
	string m_enable_resource;
	string m_polling_resource;

	long m_id;
	long m_version;
	unsigned long m_polling_interval;

	unsigned int m_bio_raw[3];

	unsigned long long m_fired_time;

	int m_client;
	int m_sensor_type;
	int m_node_handle;
	bool m_sensorhub_supported;

	cmutex m_value_mutex;


	bool update_value(bool wait);
	bool get_config(const string& element, const string& attr, string& value);
	bool get_config(const string& element, const string& attr, double& value);
	bool get_config(const string& element, const string& attr, long& value);
	bool enable_resource(string &resource_node, bool enable);
	bool is_sensorhub_supported(void);

};

#endif
//! End of a file
