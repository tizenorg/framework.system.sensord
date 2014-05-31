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
#include <csock.h>
#include <sf_common.h>

#include <csensor_module.h>
#include <cfilter_module.h>
#include <cprocessor_module.h>
#include <geo-processor.h>

#include <vconf.h>

#include <algorithm>

using std::bind1st;
using std::mem_fun;

geo_processor::geo_processor()
: m_acc_processor(NULL)
, m_geo_sensor(NULL)
, m_x(-1)
, m_y(-1)
, m_z(-1)
, m_g_hdst(-1)
, m_version(1)
, m_id(0x04BE)
, m_client(0)
, m_polling_interval(200)
{
	vector<unsigned int> supported_events = {
		GEOMAGNETIC_EVENT_CALIBRATION_NEEDED,
		GEOMAGNETIC_EVENT_ATTITUDE_DATA_REPORT_ON_TIME,
		GEOMAGNETIC_EVENT_RAW_DATA_REPORT_ON_TIME,
	};

	for_each(supported_events.begin(), supported_events.end(),
		bind1st(mem_fun(&cprocessor_module::register_supported_event), this));

	cprocessor_module::set_main(working, stopped, this);
	INFO("geo_processor is created!\n");
}



geo_processor::~geo_processor()
{
	INFO("geo_processor is destroyed!\n");
}

bool geo_processor::add_input(csensor_module *sensor)
{
	if (sensor->get_sensor_type() == ID_GEOMAG) {
		m_geo_sensor = sensor;
		DBG("Geo sensor Added");
	} else {
		ERR("cannot Add sensor module");
		return false;
	}
	return true;
}

bool geo_processor::add_input(cprocessor_module *processor)
{
    if (processor->get_processor_type() == ID_ACCEL) {
		DBG("Accel sensor Added");
        m_acc_processor = processor;
    } else {
        ERR("cannot Add processor module");
        return false;
    }
    return true;
}

const char *geo_processor::name(void)
{
	return m_name.c_str();
}

int geo_processor::id(void)
{
	return m_id;
}

int geo_processor::version(void)
{
	return m_version;
}

int geo_processor::get_processor_type(void)
{
    if (m_geo_sensor)
        return m_geo_sensor->get_sensor_type();

    return ID_ACCEL;
}

bool geo_processor::update_name(char *name)
{
	m_name = name;
	return true;
}

bool geo_processor::update_id(int id)
{
	m_id = id;
	return true;
}

bool geo_processor::update_version(int version)
{
	m_version = version;
	return true;
}

cprocessor_module *geo_processor::create_new(void)
{
	return (cprocessor_module*)this;
}

void geo_processor::destroy(cprocessor_module *module)
{
	bool bstate = false;

	bstate = cmodule::del_from_list((cmodule *)module);

	if ( !bstate ) {
		ERR("Destory and del_from_list fail");
		delete (geo_processor *)module;
		return;
	}
}

void *geo_processor::stopped(void *inst)
{
	return (void*)NULL;
}

void *geo_processor::working(void *inst)
{
	geo_processor *processor = (geo_processor*)inst;
	return (void*)(processor->process_event());
}

bool geo_processor::start(void)
{
	AUTOLOCK(m_mutex);

	if (m_client > 0) {
		m_client++;
		INFO("%s processor fake starting\n", m_name.c_str());
		return true;
	}

	INFO("%s processor real starting\n", m_name.c_str());

	m_acc_processor->add_client(ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_acc_processor->add_interval((int)this, m_polling_interval, true);

	if (!m_acc_processor->start()) {
		ERR("m_acc_processor start fail\n");
		return false;
	}

	if (!m_geo_sensor->start()) {
		ERR("m_geo_sensor start fail\n");
		return false;
	}

	cprocessor_module::start();
	m_client = 1;

	return true;
}

bool geo_processor::stop(void)
{
	AUTOLOCK(m_mutex);

	if (m_client > 1) {
		m_client--;
		INFO("%s processor fake Stopping\n", m_name.c_str());
		return true;
	}

	INFO("%s processor real Stopping\n", m_name.c_str());

	if (!cprocessor_module::stop()) {
		ERR("cprocessor_module::stop()\n");
		return false;
	}

	if (!m_acc_processor->stop()) {
		ERR("m_sensor stop fail\n");
		return false;
	}

	m_acc_processor->delete_client(ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_acc_processor->delete_interval((int)this, true);

	if (!m_geo_sensor->stop()) {
		ERR("m_sensor stop fail\n");
		return false;
	}

	m_client = 0;
	return true;
}

long geo_processor::set_cmd(int type , int property , long input_value)
{
	return -1;
}

int geo_processor::get_property(unsigned int property_level , void *property_data )
{
	DBG("geo_processor called get_property , with property_level : 0x%x", property_level);
	if (m_geo_sensor) {
		INFO("property level = [0x%X]!!", property_level);
		return m_geo_sensor->get_property(property_level , property_data);
	} else {
		ERR("no m_sensor , cannot get_struct_value from sensor\n");
		return -1;
	}
}

int geo_processor::get_struct_value(unsigned int struct_type , void *struct_values)
{
	if (m_geo_sensor) {
		return m_geo_sensor->get_struct_value(struct_type , struct_values);
	} else {
		ERR("no m_sensor , cannot get_struct_value from sensor\n");
		return -1;
	}
}

bool geo_processor::update_polling_interval(unsigned long interval)
{
	AUTOLOCK(m_mutex);

	INFO("Polling interval is set to %dms", interval);

	m_polling_interval = interval;

	if (m_client > 0)
		m_acc_processor->add_interval((int)this, m_polling_interval, true);

	return m_geo_sensor->update_polling_interval(interval);
}

void geo_processor::pass_accel_data_to_sensor(void)
{
	const int SIG_FIGS = 1000;

	base_data_struct accel_base_data;
        m_acc_processor->get_struct_value(ACCELEROMETER_BASE_DATA_SET, &accel_base_data);

	m_geo_sensor->set_cmd(ID_GEOMAG, PROPERTY_CMD_SET_ACCEL_X, (long)(accel_base_data.values[0] * SIG_FIGS));
	m_geo_sensor->set_cmd(ID_GEOMAG, PROPERTY_CMD_SET_ACCEL_Y, (long)(accel_base_data.values[1] * SIG_FIGS));
	m_geo_sensor->set_cmd(ID_GEOMAG, PROPERTY_CMD_SET_ACCEL_Z, (long)(accel_base_data.values[2] * SIG_FIGS));

}

int geo_processor::process_event(void)
{
	sensor_event_t event;

	if (!m_acc_processor) {
		ERR("Accel sensor is not added\n");
		return cworker::STOPPED;
	}

	if (!m_geo_sensor) {
		ERR("Geomagnetic sensor is not added\n");
		return cworker::STOPPED;
	}

	pass_accel_data_to_sensor();

	if (m_geo_sensor->is_data_ready(true) == false) {
		return cworker::STARTED;
	}

	base_data_struct base_data;
	m_geo_sensor->get_struct_value(GEOMAGNETIC_RAW_DATA_SET, &base_data);

	base_data_struct attitude_data;
	m_geo_sensor->get_struct_value(GEOMAGNETIC_ATTITUDE_DATA_SET, &attitude_data);

	AUTOLOCK(m_client_info_mutex);

	if (get_client_cnt(GEOMAGNETIC_EVENT_RAW_DATA_REPORT_ON_TIME)) {

		event.event_type = GEOMAGNETIC_EVENT_RAW_DATA_REPORT_ON_TIME;
		base_data_to_sensor_event(&base_data, &event);
		push(event);
	}

	if (get_client_cnt(GEOMAGNETIC_EVENT_ATTITUDE_DATA_REPORT_ON_TIME)) {

		event.event_type = GEOMAGNETIC_EVENT_ATTITUDE_DATA_REPORT_ON_TIME;
		base_data_to_sensor_event(&attitude_data, &event);
		push(event);
	}

	AUTOLOCK(m_value_mutex);
	m_x = base_data.values[0];
	m_y = base_data.values[1];
	m_z = base_data.values[2];
	m_g_hdst = attitude_data.values[0];

	return cworker::STARTED;
}

cmodule *module_init(void *win, void *egl)
{
	geo_processor *inst;

	try {
		inst = new geo_processor();
	} catch (int ErrNo) {
		ERR("geo_processor class create fail , errno : %d , errstr : %s\n",ErrNo, strerror(ErrNo));
		return NULL;
	}

	return (cmodule*)inst;
}

void module_exit(cmodule *inst)
{
	geo_processor *sample = (geo_processor*)inst;
	delete sample;
}
