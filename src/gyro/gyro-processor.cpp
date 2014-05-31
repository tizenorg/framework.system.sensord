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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include <pthread.h>
#include <string.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <math.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <dirent.h>

#include <common.h>
#include <cobject_type.h>
#include <clist.h>
#include <cmutex.h>
#include <cmodule.h>
#include <cworker.h>
#include <cpacket.h>
#include <sf_common.h>

#include <csensor_module.h>
#include <cfilter_module.h>
#include <cprocessor_module.h>
#include <gyro-processor.h>

#include <vconf.h>

#define MS_TO_US 1000
#define DPS_TO_MDPS 1000
#define RAW_DATA_TO_DPS_UNIT(X) ((float)(X)/((float)DPS_TO_MDPS))

gyro_processor::gyro_processor()
: m_sensor(NULL)
, m_x(-1)
, m_y(-1)
, m_z(-1)
, m_version(1)
, m_id(0x04BE)
, m_client(0)
{
	register_supported_event(GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME);

	cprocessor_module::set_main(working, stopped, this);

	INFO("gyro_processor is created!\n");
}



gyro_processor::~gyro_processor()
{
	INFO("gyro_processor is destroyed!\n");
}



bool gyro_processor::add_input(csensor_module *sensor)
{
	m_sensor = sensor;

	base_property_struct property_data;

	if (sensor->get_property(GYRO_BASE_DATA_SET, &property_data) != 0) {
		ERR("sensor->get_property() is failed!\n");
		return false;
	}

	m_resolution = property_data.sensor_resolution;
	return true;

}

bool gyro_processor::add_input(cprocessor_module *processor)
{
    return true;
}

const char *gyro_processor::name(void)
{
	return m_name.c_str();
}



int gyro_processor::id(void)
{
	return m_id;
}



int gyro_processor::version(void)
{
	return m_version;
}



int gyro_processor::get_processor_type(void)
{
	if (m_sensor)
		return m_sensor->get_sensor_type();
	else
		return ID_GYROSCOPE;
}
bool gyro_processor::update_name(char *name)
{
	m_name = name;
	return true;
}



bool gyro_processor::update_id(int id)
{
	m_id = id;
	return true;
}

bool gyro_processor::update_version(int version)
{
	m_version = version;
	return true;
}



cprocessor_module *gyro_processor::create_new(void)
{
	return (cprocessor_module*)this;
}

void gyro_processor::destroy(cprocessor_module *module)
{
	bool bstate = false;

	bstate = cmodule::del_from_list((cmodule *)module);

	if ( !bstate ) {
		ERR("Destory and del_from_list fail");
		delete (gyro_processor *)module;
		return ;
	}
}



void *gyro_processor::stopped(void *inst)
{
	return (void*)NULL;
}



void *gyro_processor::working(void *inst)
{
	gyro_processor *processor = (gyro_processor*)inst;
	return (void*)(processor->process_event());
}

bool gyro_processor::start(void)
{
	bool ret;
	AUTOLOCK(m_mutex);

	if (m_client > 0) {
		m_client++;
		INFO("%s processor fake starting\n", m_name.c_str());
		return true;
	}

	INFO("%s processor real starting\n", m_name.c_str());

	ret = m_sensor ? m_sensor->start() : true;
	if ( ret != true ) {
		ERR("m_sensor start fail\n");
		return false;
	}

	cprocessor_module::start();
	m_client = 1;
	return ret;
}



bool gyro_processor::stop(void)
{
	bool ret;

	AUTOLOCK(m_mutex);
	if (m_client > 1) {
		m_client--;
		INFO("%s processor fake Stopping\n",m_name.c_str());
		return true;
	}

	INFO("%s processor real Stopping\n",m_name.c_str());

	ret = cprocessor_module::stop();
	if ( ret != true ) {
		ERR("cprocessor_module::stop()\n");
		return false;
	}

	ret = m_sensor ? m_sensor->stop() : true;
	if ( ret != true ) {
		ERR("m_sensor stop fail\n");
		return false;
	}

	m_client = 0;
	return ret;
}


long gyro_processor::set_cmd(int type , int property , long input_value)
{
	return -1;
}

int gyro_processor::get_property(unsigned int property_level , void *property_data )
{
	DBG("gyro_processor called get_property , with property_level : 0x%x", property_level);
	if (m_sensor) {
		DBG("property level = [0x%X]!!", property_level);
		return m_sensor->get_property(property_level , property_data);
	} else {
		ERR("no m_sensor , cannot get_struct_value from sensor\n");
		return -1;
	}
}

int gyro_processor::get_struct_value(unsigned int struct_type , void *struct_values)
{
	int state;
	base_data_struct sensor_struct_data;
	base_data_struct *return_struct_data;

	return_struct_data = (base_data_struct *)struct_values;

	state = m_sensor ? m_sensor->get_struct_value(GYRO_BASE_DATA_SET, &sensor_struct_data) : -1;
	if (state < 0) {
		ERR("Error , m_sensor get struct_data fail\n");
		return -1;
	}

	if (struct_type == GYRO_BASE_DATA_SET)
		raw_to_base(&sensor_struct_data, return_struct_data);
	else {
		ERR("does not support stuct_type\n");
		return -1;
	}

	return 0;
}

bool gyro_processor::update_polling_interval(unsigned long interval)
{
	INFO("Polling interval is set to %dms", interval);

	return m_sensor->update_polling_interval(interval);
}

void gyro_processor::raw_to_base(base_data_struct *raw, base_data_struct *base)
{
	base->time_stamp = raw->time_stamp;
	base->data_accuracy = raw->data_accuracy;
	base->data_unit_idx = IDX_UNIT_DEGREE_PER_SECOND;
	base->values_num = 3;
	base->values[0] = raw->values[0] * m_resolution;
	base->values[1] = raw->values[1] * m_resolution;
	base->values[2] = raw->values[2] * m_resolution;
}

int gyro_processor::process_event(void)
{
	float x, y, z;
	sensor_event_t event;

	if (!m_sensor) {
		ERR("Sensor is not added\n");
		return cworker::STOPPED;
	}

	if (m_sensor->is_data_ready(true) == false) {
		return cworker::STARTED;
	}

	base_data_struct raw_data;
	m_sensor->get_struct_value(GYRO_BASE_DATA_SET, &raw_data);

	x = raw_data.values[0];
	y = raw_data.values[1];
	z = raw_data.values[2];
	event.timestamp = raw_data.time_stamp;

	AUTOLOCK(m_client_info_mutex);

	if (get_client_cnt(GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME)) {
		base_data_struct base_data;
		raw_to_base(&raw_data, &base_data);

		event.event_type = GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME;
		base_data_to_sensor_event(&base_data, &event);
		push(event);
	}


	AUTOLOCK(m_value_mutex);
	m_x = x;
	m_y = y;
	m_z = z;

	return cworker::STARTED;
}

cmodule *module_init(void *win, void *egl)
{
	gyro_processor *inst;

	try {
		inst = new gyro_processor();
	} catch (int ErrNo) {
		ERR("gyro_processor class create fail , errno : %d , errstr : %s\n",ErrNo, strerror(ErrNo));
		return NULL;
	}

	return (cmodule*)inst;
}



void module_exit(cmodule *inst)
{
	gyro_processor *sample = (gyro_processor*)inst;
	delete sample;
}
