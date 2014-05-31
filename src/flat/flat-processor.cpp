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
#include <time.h>


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
#include <flat-processor.h>

#include <vconf.h>

#include <algorithm>

using std::bind1st;
using std::mem_fun;

#define PROCESSOR_NAME "flat_processor"

flat_processor::flat_processor()
: m_sensor(NULL)
, m_version(1)
, m_id(0x04BE)
, m_client(0)
, m_flat_state(FLAT_UNKNOWN)
, m_flat_state_cb_client(0)

{
	m_name = PROCESSOR_NAME;

	vector<unsigned int> supported_events = {
		FLAT_EVENT_CHANGE_STATE,
	};

	for_each(supported_events.begin(), supported_events.end(),
		bind1st(mem_fun(&cprocessor_module::register_supported_event), this));


	cprocessor_module::set_main(working, stopped, this);

	INFO("flat_processor is created!\n");
}


flat_processor::~flat_processor()
{
	INFO("flat_processor is created!\n");
}



bool flat_processor::add_input(csensor_module *sensor)
{
	m_sensor = sensor;
	return true;
}


bool flat_processor::add_input(cprocessor_module *processor)
{
    return true;
}


const char *flat_processor::name(void)
{
	return m_name.c_str();
}



int flat_processor::id(void)
{
	return m_id;
}



int flat_processor::version(void)
{
	return m_version;
}



int flat_processor::get_processor_type(void)
{
	if (m_sensor)
		return m_sensor->get_sensor_type();

	return ID_FLAT;
}


bool flat_processor::update_name(char *name)
{
	m_name = name;
	return true;
}



bool flat_processor::update_id(int id)
{
	m_id = id;
	return true;
}



bool flat_processor::update_version(int version)
{
	m_version = version;
	return true;
}



cprocessor_module *flat_processor::create_new(void)
{
	return (cprocessor_module*)this;
}



void flat_processor::destroy(cprocessor_module *module)
{
	bool bstate = false;

	bstate = cmodule::del_from_list((cmodule *)module);

	if ( !bstate ) {
		ERR("Destory and del_from_list fail");
		delete (flat_processor *)module;
		return ;
	}
}

void *flat_processor::stopped(void *inst)
{
	return (void*)NULL;
}



void *flat_processor::working(void *inst)
{
	flat_processor *processor = (flat_processor*)inst;
	return (void*)(processor->process_event());
}

bool flat_processor::start(void)
{
	bool ret;

	AUTOLOCK(m_mutex);
	if (m_client > 0) {
		m_client++;
		DBG("%s processor fake starting\n",m_name.c_str());
		return true;
	}

	INFO("%s processor real starting\n",m_name.c_str());

	ret = m_sensor ? m_sensor->start() : true;
	if ( ret != true ) {
		ERR("m_sensor start fail\n");
		return false;
	}

	ret = cprocessor_module::start();
	m_client = 1;
	m_flat_state = FLAT_UNKNOWN;
	return ret;
}



bool flat_processor::stop(void)
{
	bool ret;

	AUTOLOCK(m_mutex);
	if (m_client > 1) {
		m_client--;
		DBG("%s processor fake Stopping\n",m_name.c_str());
		return true;
	}

	INFO("%s processor real Stopping\n",m_name.c_str());

	ret = cprocessor_module::stop();

	if (ret != true) {
		ERR("cprocessor_module::stop() fail!\n");
		return false;
	}

	ret = m_sensor ? m_sensor->stop() : true;
	if ( ret != true ) {
		ERR("m_sensor stop fail\n");
		return false;
	}

	m_client = 0;
	return true;
}

long flat_processor::set_cmd(int type , int property , long input_value)
{
	ERR("set_cmd() is not supported!!");
	return -1;
}

int flat_processor::get_property(unsigned int property_level , void *property_data )
{
	if(m_sensor) {
		return m_sensor->get_property(property_level , property_data);
	}
	else {
		ERR("no m_sensor , cannot get_property from sensor\n");
		return -1;
	}
}

int flat_processor::get_struct_value(unsigned int struct_type , void *struct_values)
{
	if (m_sensor) {
		return m_sensor->get_struct_value(struct_type , struct_values);
	}
	else {
		ERR("no m_sensor , cannot get_struct_value from sensor\n");
		return -1;
	}
}

bool flat_processor::update_polling_interval(unsigned long interval)
{
	INFO("Polling interval is set to %dms", interval);

	return m_sensor->update_polling_interval(interval);
}


int flat_processor::process_event(void)
{

	sensor_event_t event;
	int state;

	if (!m_sensor) {
		ERR("Sensor is not added\n");
		return cworker::STOPPED;
	}


	if (m_sensor->is_data_ready(true) == false)
		return cworker::STARTED;

	base_data_struct base_data;
	m_sensor->get_struct_value(FLAT_BASE_DATA_SET, &base_data);

	AUTOLOCK(m_client_info_mutex);

	state = base_data.values[0];

	if (m_flat_state != state) {
		AUTOLOCK(m_value_mutex);
		m_flat_state = state;

		if (get_client_cnt(FLAT_EVENT_CHANGE_STATE)) {
			event.event_type = FLAT_EVENT_CHANGE_STATE;
			event.values_num = 1;
			event.timestamp = base_data.time_stamp;
			event.values[0] = state;
			push(event);
		}
	}

	return cworker::STARTED;

}


cmodule *module_init(void *win, void *egl)
{
	flat_processor *inst;

	try {
		inst = new flat_processor();
	} catch (int ErrNo) {
		ERR("flat_processor class create fail , errno : %d , errstr : %s\n",ErrNo, strerror(ErrNo));
		return NULL;
	}

	return (cmodule*)inst;
}



void module_exit(cmodule *inst)
{
	flat_processor *sample = (flat_processor*)inst;
	delete sample;
}

