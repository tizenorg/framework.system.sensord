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

#if !defined(_GEO_PROCESSOR_CLASS_H_)
#define _GEO_PROCESSOR_CLASS_H_

#include <sensor.h>
#include <string>

using std::string;

class geo_processor : public cprocessor_module
{
public:
	enum geo_calibration_event_t {
		HDST_UNSOLVED = 0,
		HDST_LEVEL0 = 1,
		HDST_LEVEL1 = 2,
		HDST_LEVEL2 = 3,
	};

	enum geo_cmd_property_t {
		PROPERTY_CMD_START = 0,
		PROPERTY_CMD_SET_ACCEL_X,
		PROPERTY_CMD_SET_ACCEL_Y,
		PROPERTY_CMD_SET_ACCEL_Z,
		PROPERTY_CMD_SET_CALIBRATION,
	};

	geo_processor();
	virtual ~geo_processor();

	const char *name(void);
	int id(void);
	int version(void);
	int get_processor_type(void);

	bool update_name(char *name);
	bool update_id(int id);
	bool update_version(int version);

	bool add_input(csensor_module *sensor);
	bool add_input(cprocessor_module *processor);

	cprocessor_module *create_new(void);
	void destroy(cprocessor_module *module);

	static void *working(void *inst);
	static void *stopped(void *inst);

	virtual bool start(void);
	virtual bool stop(void);


	long set_cmd(int type, int property, long input_value);
	int get_property(unsigned int property_level, void *property_data);
	int get_struct_value(unsigned int struct_type, void *struct_values);


protected:
	bool update_polling_interval(unsigned long interval);

private:
	cprocessor_module *m_acc_processor;
	csensor_module *m_geo_sensor;

	long m_x;
	long m_y;
	long m_z;

	int m_g_hdst;

	long m_version;
	long m_id;

	string m_name;

	int m_client;
	unsigned long m_polling_interval;

	cmutex m_mutex;
	cmutex m_value_mutex;;

	void pass_accel_data_to_sensor(void);
	int process_event(void);
};
#endif
