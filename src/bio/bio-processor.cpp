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
#include <bio-processor.h>

#include <vconf.h>

#define MS_TO_US 1000

bio_processor::bio_processor()
: m_sensor(NULL)
, m_version(1)
, m_id(0x04BE)
, m_client(0)
{
	memset(m_bio_raw, 0, sizeof(m_bio_raw));

	register_supported_event(BIO_EVENT_RAW_DATA_REPORT_ON_TIME);

	cprocessor_module::set_main(working, stopped, this);

	INFO("bio_processor is created!\n");
}

bio_processor::~bio_processor()
{
	INFO("bio_processor is destroyed!\n");
}



bool bio_processor::add_input(csensor_module *sensor)
{
	m_sensor = sensor;
	return true;
}

bool bio_processor::add_input(cprocessor_module *processor)
{
    return true;
}

const char *bio_processor::name(void)
{
	return m_name.c_str();
}



int bio_processor::id(void)
{
	return m_id;
}



int bio_processor::version(void)
{
	return m_version;
}



int bio_processor::get_processor_type(void)
{
	if (m_sensor)
		return m_sensor->get_sensor_type();
	else
		return ID_BIO;
}
bool bio_processor::update_name(char *name)
{
	m_name = name;
	return true;
}



bool bio_processor::update_id(int id)
{
	m_id = id;
	return true;
}

bool bio_processor::update_version(int version)
{
	m_version = version;
	return true;
}



cprocessor_module *bio_processor::create_new(void)
{
	return (cprocessor_module*)this;
}

void bio_processor::destroy(cprocessor_module *module)
{
	bool bstate = false;

	bstate = cmodule::del_from_list((cmodule *)module);

	if ( !bstate ) {
		ERR("Destory and del_from_list fail");
		delete (bio_processor *)module;
		return ;
	}
}



void *bio_processor::stopped(void *inst)
{
	return (void*)NULL;
}



void *bio_processor::working(void *inst)
{
	bio_processor *processor = (bio_processor*)inst;
	return (void*)(processor->process_event());
}

bool bio_processor::start(void)
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



bool bio_processor::stop(void)
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


long bio_processor::set_cmd(int type , int property , long input_value)
{
	return -1;
}

int bio_processor::get_property(unsigned int property_level , void *property_data )
{
	DBG("bio_processor called get_property , with property_level : 0x%x", property_level);
	if (m_sensor) {
		DBG("property level = [0x%X]!!", property_level);
		return m_sensor->get_property(property_level , property_data);
	} else {
		ERR("no m_sensor , cannot get_struct_value from sensor\n");
		return -1;
	}
}

int bio_processor::get_struct_value(unsigned int struct_type , void *struct_values)
{
	int state;
	base_data_struct *return_struct_data;

	if (struct_type != BIO_BASE_DATA_SET) {
		ERR("does not support stuct_type\n");
		return -1;
	}

	return_struct_data = (base_data_struct *)struct_values;

	state = m_sensor ? m_sensor->get_struct_value(BIO_BASE_DATA_SET, return_struct_data) : -1;
	if (state < 0) {
		ERR("Error , m_sensor get struct_data fail\n");
		return -1;
	}

	return 0;
}

bool bio_processor::update_polling_interval(unsigned long interval)
{
	INFO("Polling interval is set to %dms", interval);

	return m_sensor->update_polling_interval(interval);
}

int bio_processor::process_event(void)
{
	sensor_event_t event;

	if (!m_sensor) {
		ERR("Sensor is not added\n");
		return cworker::STOPPED;
	}

	if (m_sensor->is_data_ready(true) == false) {
		return cworker::STARTED;
	}

	base_data_struct base_data;
	m_sensor->get_struct_value(BIO_BASE_DATA_SET, &base_data);

	event.timestamp = base_data.time_stamp;

	AUTOLOCK(m_client_info_mutex);

	if (get_client_cnt(BIO_EVENT_RAW_DATA_REPORT_ON_TIME)) {
		event.event_type = BIO_EVENT_RAW_DATA_REPORT_ON_TIME;
		base_data_to_sensor_event(&base_data, &event);
		push(event);
	}

	AUTOLOCK(m_value_mutex);
	m_bio_raw[0] = (unsigned int) base_data.values[0];
	m_bio_raw[1] = (unsigned int) base_data.values[1];
	m_bio_raw[2] = (unsigned int) base_data.values[2];

	return cworker::STARTED;
}

cmodule *module_init(void *win, void *egl)
{
	bio_processor *inst;

	try {
		inst = new bio_processor();
	} catch (int ErrNo) {
		ERR("bio_processor class create fail , errno : %d , errstr : %s\n",ErrNo, strerror(ErrNo));
		return NULL;
	}

	return (cmodule*)inst;
}



void module_exit(cmodule *inst)
{
	bio_processor *sample = (bio_processor*)inst;
	delete sample;
}
