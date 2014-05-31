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

#if !defined(_LIGHT_PROCESSOR_CLASS_H_)
#define _LIGHT_PROCESSOR_CLASS_H_
#include <sensor.h>
#include <string>

using std::string;

class light_processor:public cprocessor_module {
 public:
	 light_processor();
	 virtual ~ light_processor();

	const char *name(void);
	int id(void);
	int version(void);
	int get_processor_type(void);

	bool update_name(char *name);
	bool update_id(int id);
	bool update_version(int version);

	bool add_input(csensor_module * sensor);
	bool add_input(cprocessor_module * processor);

	cprocessor_module *create_new(void);
	void destroy(cprocessor_module * module);

	static void *working(void *inst);
	static void *stopped(void *inst);

	virtual bool start(void);
	virtual bool stop(void);

	long set_cmd(int type, int property, long input_value);
	int check_hw_node(void);

	int get_property(unsigned int property_level, void *property_data);
	int get_struct_value(unsigned int struct_type, void *struct_values);
protected:
	bool update_polling_interval(unsigned long interval);

private:
	csensor_module * m_sensor;

	int m_level;

	long m_version;
	long m_id;

	string m_name;

	int m_client;

	cmutex m_mutex;
	cmutex m_value_mutex;

	int process_event(void);
};

#endif
