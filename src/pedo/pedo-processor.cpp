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
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <dirent.h>
#include <dlfcn.h>

#include <common.h>
#include <cobject_type.h>
#include <clist.h>
#include <cmutex.h>
#include <cmodule.h>
#include <cworker.h>
#include <cpacket.h>
#include <csock.h>
#include <sf_common.h>

#include <csensor_module.h>
#include <cfilter_module.h>
#include <cprocessor_module.h>
#include <pedo-processor.h>

#include <vconf.h>

#define PROCESSOR_NAME "pedo_processor"

#define PORT_COUNT 1

#define NAME_SUFFIX "_processor"

pedo_processor::pedo_processor()
: m_sensor(NULL)
, m_step_cnt(0)
, m_version(1)
, m_id(0x04BE)
, m_client(0)
, m_event_callback_state(0)
{
	m_name = PROCESSOR_NAME;

	register_supported_event(PEDOMETER_EVENT_STEP_COUNT);

	cprocessor_module::set_main(working, stopped, this);
	INFO("pedo_processor is created!\n");
}



pedo_processor::~pedo_processor()
{
	INFO("pedo_processor is destroyed!\n");
}


bool pedo_processor::add_input(csensor_module *sensor)
{
	m_sensor = sensor;
	m_name = string(sensor->name()) + string(NAME_SUFFIX);
	return true;
}


bool pedo_processor::add_input(cprocessor_module *processor)
{
    return true;
}

const char *pedo_processor::name(void)
{
	return m_name.c_str();
}



int pedo_processor::id(void)
{
	return m_id;
}

int pedo_processor::version(void)
{
	return m_version;
}

int pedo_processor::get_processor_type(void)
{
	if (m_sensor)
		return m_sensor->get_sensor_type();

	return ID_PEDO;
}


bool pedo_processor::update_name(char *name)
{
	m_name = name;
	return true;
}



bool pedo_processor::update_id(int id)
{
	m_id = id;
	return true;
}



bool pedo_processor::update_version(int version)
{
	m_version = version;
	return true;
}



cprocessor_module *pedo_processor::create_new(void)
{
	return (cprocessor_module*)this;
}



void pedo_processor::destroy(cprocessor_module *module)
{
	bool bstate = false;

	bstate = cmodule::del_from_list((cmodule *)module);

	if (!bstate) {
		ERR("Destory and del_from_list fail");
		delete (pedo_processor *)module;
		return ;
	}
}

void *pedo_processor::stopped(void *inst)
{
	return (void*)NULL;
}

void *pedo_processor::working(void *inst)
{
	pedo_processor *processor = (pedo_processor*)inst;
	return (void*)(processor->process_event());
}

bool pedo_processor::start(void)
{
	AUTOLOCK(m_mutex);

	if (m_client < 0) {
		ERR("%s processor has wrong client cnt: %d\n", m_name.c_str(), m_client);
		return false;
	}

	++m_client;

	if (m_client == 1) {
		bool ret;
		ret = m_sensor ? m_sensor->start() : false;
		if ( ret != true ) {
			ERR("m_sensor start fail\n");
			return false;
		}
	}

	if (m_event_callback_state) {
		if (cprocessor_module::start() == true)
			INFO("%s processor starts working", m_name.c_str());
	}

	return true;
}

bool pedo_processor::stop(void)
{
	AUTOLOCK(m_mutex);

	if (m_client < 0) {
		ERR("%s processor has wrong client cnt: %d\n", m_name.c_str(), m_client);
		return false;
	}

	if (!m_event_callback_state) {
		if (cprocessor_module::stop() == true)
			INFO("%s processor stops working", m_name.c_str());
	}

	--m_client;

	if (m_client == 0) {
		bool ret;
		ret = m_sensor ? m_sensor->stop() : false;
		if (ret != true) {
			ERR("m_sensor stop fail\n");
			return false;
		}
	}

	return true;
}

bool pedo_processor::add_client(unsigned int event_type)
{
	AUTOLOCK(m_mutex);
	AUTOLOCK(m_client_info_mutex);

	if (!is_supported(event_type)) {
		ERR("invaild event type: 0x%x !!", event_type);
		return false;
	}

	cprocessor_module::add_client(event_type);

	switch (event_type) {
	case PEDOMETER_EVENT_STEP_COUNT:
		if (get_client_cnt(PEDOMETER_EVENT_STEP_COUNT) == 1) {
			m_event_callback_state |= (PEDOMETER_EVENT_STEP_COUNT & 0x0000FFFF);
			if (m_client >= 1) {
				if (cprocessor_module::start())
					INFO("%s processor starts working", m_name.c_str());
			}
		}
		break;
	default:
		break;
	}

	return true;
}

bool pedo_processor::delete_client(unsigned int event_type)
{
	AUTOLOCK(m_client_info_mutex);

	if (!is_supported(event_type)) {
		ERR("invaild event type: 0x%x!!", event_type);
		return false;
	}

	if (!cprocessor_module::delete_client(event_type)) {
		ERR("Invaild event type: 0x%x!!", event_type);
		return false;
	}

	switch (event_type) {
	case PEDOMETER_EVENT_STEP_COUNT:
		if (get_client_cnt(PEDOMETER_EVENT_STEP_COUNT) == 0) {
			m_event_callback_state ^= (PEDOMETER_EVENT_STEP_COUNT & 0x0000FFFF);
			if (cprocessor_module::stop())
				INFO("%s processor stops working", m_name.c_str());
		}
		break;

	default:
		break;
	}

	return true;
}



long pedo_processor::set_cmd(int type , int property , long input_value)
{
	int ret = 0;

	ret = m_sensor ? m_sensor->set_cmd(type, property, input_value) : 0;
	if (ret < 0) {
		ERR("m_sensor set_cmd fail");
		return ret;
	}

	DBG("ret = [%d] property = [%d]",ret, property);
	return ret;
}

int pedo_processor::get_property(unsigned int property_level , void *property_data )
{
	int result = -1;
	base_property_struct *return_property;
	return_property = (base_property_struct *)property_data;

	if (m_sensor) {
		result = m_sensor->get_property(PEDOMETER_BASE_DATA_SET, return_property);
		if (result == 0)	{
			if (property_level == PEDOMETER_BASE_DATA_SET) {
				return result;
			} else {
				ERR("cannot get_property from sensor\n");
				return -1;
			}

			return result;
		}
	} else {
		ERR("no m_sensor , cannot get_property from sensor\n");
	}

	return -1;
}

int pedo_processor::get_struct_value(unsigned int struct_type , void *struct_values)
{
	int state;
	base_data_struct sensor_struct_data;
	base_data_struct *return_struct_data = NULL;

	if (!m_sensor) {
		ERR("Sensor is not added\n");
		return -1;
	}

	state = m_sensor->get_struct_value(PEDOMETER_BASE_DATA_SET , &sensor_struct_data);

	if (state < 0) {
		ERR("Error , m_sensor get struct_data fail\n");
		return -1;
	}

	if (struct_type == PEDOMETER_BASE_DATA_SET) {
		return_struct_data = (base_data_struct *)struct_values;
		*return_struct_data = sensor_struct_data;
	} else {
		ERR("does not support stuct_type\n");
		return -1;
	}

	return 0;

}


bool pedo_processor::update_polling_interval(unsigned long interval)
{
	INFO("Polling interval is set to %dms", interval);

	return m_sensor->update_polling_interval(interval);
}

int pedo_processor::process_event(void)
{
	unsigned int cur_step_cnt = 0;
	sensor_event_t event;

	if (!m_sensor) {
		ERR("Sensor is not added\n");
		return cworker::STOPPED;
	}

	if (m_sensor->is_data_ready(true) == false)
		return cworker::STARTED;

	base_data_struct base_data;
	m_sensor->get_struct_value(PEDOMETER_EVENT_STEP_COUNT, &base_data);

	cur_step_cnt = base_data.values[0];

	AUTOLOCK(m_client_info_mutex);

	if (cur_step_cnt != m_step_cnt) {
		AUTOLOCK(m_value_mutex);
		m_step_cnt = cur_step_cnt;
		DBG("Pedo step cnt is updated. count: %u\n", cur_step_cnt);

		if (get_client_cnt(PEDOMETER_EVENT_STEP_COUNT)) {
			event.event_type = PEDOMETER_EVENT_STEP_COUNT;
			base_data_to_sensor_event(&base_data, &event);
			push(event);
		}

	}

	return cworker::STARTED;
}



cmodule *module_init(void *win, void *egl)
{
	pedo_processor *inst;

	try {
		inst = new pedo_processor();
	} catch (int ErrNo) {
		ERR("pedo_processor class create fail , errno : %d , errstr : %s\n",ErrNo, strerror(ErrNo));
		return NULL;
	}

	return (cmodule*)inst;
}

void module_exit(cmodule *inst)
{
	pedo_processor *sample = (pedo_processor*)inst;
	delete sample;
}
