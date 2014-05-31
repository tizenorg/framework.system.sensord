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
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <netinet/in.h>
#include <dirent.h>
#include <math.h>
#include <common.h>
#include <cobject_type.h>
#include <cmutex.h>
#include <clist.h>
#include <cmodule.h>
#include <cpacket.h>
#include <cworker.h>
#include <csock.h>
#include <sf_common.h>

#include <csensor_module.h>

#include <proxi-sensor.h>
#include <errno.h>


#include <cconfig.h>
#include <fstream>

using std::ifstream;

#define SENSOR_TYPE_PROXI		"PROXI"
#define ELEMENT_NAME			"NAME"
#define ELEMENT_VENDOR			"VENDOR"
#define ELEMENT_RAW_DATA_UNIT	"RAW_DATA_UNIT"
#define ELEMENT_RESOLUTION		"RESOLUTION"

#define ATTR_VALUE				"value"


const char *proxi_sensor::m_port[] = {"none","enable","state"};

#define INPUT_NAME "proximity_sensor"

#define PROXI_CODE	0x0019

proxi_sensor::proxi_sensor()
: m_id(1118)
, m_version(1)
, m_polling_interval(POLL_1HZ_MS)
, m_enable(-1)
, m_state(PROXIMITY_STATE_FAR)
, m_fired_time(0)
, m_client(0)
, m_node_handle(-1)
, m_sensor_type(ID_PROXI)
, m_thr(0)
{

	if (!check_hw_node()) {
		ERR("check_hw_node() fail\n");
		throw ENXIO;
	}

	if (get_config(ELEMENT_VENDOR,ATTR_VALUE, m_vendor) == false) {
		ERR("[VENDOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_vendor = %s\n",m_vendor.c_str());

	if (get_config(ELEMENT_NAME,ATTR_VALUE, m_chip_name) == false) {
		ERR("[NAME] is empty\n");
		throw ENXIO;
	}

	if ((m_node_handle = open(m_resource.c_str(), O_RDWR)) < 0) {
		ERR("Proximity handle open fail for Proximity processor(handle:%d)\n",m_node_handle);
		throw ENXIO;
	}

	int clockId = CLOCK_MONOTONIC;
	if (ioctl(m_node_handle, EVIOCSCLOCKID, &clockId) != 0) {
		ERR("Fail to set monotonic timestamp for %s", m_resource.c_str());
		throw ENXIO;
	}

	INFO("m_chip_name = %s\n",m_chip_name.c_str());
	INFO("proxi_sensor is created!\n");
}



proxi_sensor::~proxi_sensor()
{
	close(m_node_handle);
	m_node_handle = -1;

	INFO("proxi_sensor is destroyed!\n");
}

using config::CConfig;

bool proxi_sensor::get_config(const string& element,const string& attr, string& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}

	return CConfig::getInstance().get(SENSOR_TYPE_PROXI,m_model_id,element,attr,value);
}

bool proxi_sensor::get_config(const string& element,const string& attr, double& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}

	return CConfig::getInstance().get(SENSOR_TYPE_PROXI,m_model_id,element,attr,value);

}

bool proxi_sensor::get_config(const string& element,const string& attr, long& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}
	return CConfig::getInstance().get(SENSOR_TYPE_PROXI,m_model_id,element,attr,value);
}

const char *proxi_sensor::name(void)
{
	return m_name.c_str();
}



int proxi_sensor::version(void)
{
	return m_version;
}



int proxi_sensor::id(void)
{
	return m_id;
}


bool proxi_sensor::update_value(bool wait)
{
	struct timeval tv;
	fd_set readfds,exceptfds;

	FD_ZERO(&readfds);
	FD_ZERO(&exceptfds);

	FD_SET(m_node_handle, &readfds);
	FD_SET(m_node_handle, &exceptfds);

	if (wait) {
		tv.tv_sec = m_polling_interval / 1000;
		tv.tv_usec = (m_polling_interval % 1000) * 1000;
	} else {
		tv.tv_sec = 0;
		tv.tv_usec = 0;
	}

	int ret;
	ret = select(m_node_handle+1, &readfds, NULL, &exceptfds, &tv);

	if (ret == -1) {
		ERR("select error:%s m_node_handle:d\n", strerror(errno), m_node_handle);
		return false;
	} else if (!ret) {
		DBG("select timeout: %d seconds elapsed.\n", tv.tv_sec);
		return false;
	}

	if (FD_ISSET(m_node_handle, &exceptfds)) {
			ERR("select exception occurred!\n");
			return false;
	}

	if (FD_ISSET(m_node_handle, &readfds)) {
		struct input_event proxi_event;
		DBG("proxi event detection!");

		int len;
		len = read(m_node_handle, &proxi_event, sizeof(proxi_event));

		if (len == -1) {
			DBG("read(m_node_handle) is error:%s.\n", strerror(errno));
			return false;
		}

		DBG("read event,  len : %d , type : %x , code : %x , value : %x", len, proxi_event.type, proxi_event.code, proxi_event.value);
		if ((proxi_event.type == EV_ABS) && (proxi_event.code == PROXI_CODE)) {
			AUTOLOCK(m_value_mutex);
			if (proxi_event.value == PROXIMITY_NODE_STATE_FAR) {
				INFO("PROXIMITY_STATE_FAR state occured\n");
				m_state = PROXIMITY_STATE_FAR;
			} else if (proxi_event.value == PROXIMITY_NODE_STATE_NEAR) {
				INFO("PROXIMITY_STATE_NEAR state occured\n");
				m_state = PROXIMITY_STATE_NEAR;
			} else {
				ERR("PROXIMITY_STATE Unknown: %d\n",proxi_event.value);
				return false;
			}
			m_fired_time = cmodule::get_timestamp(&proxi_event.time);
		} else {
			return false;
		}
	} else {
		ERR("select nothing to read!!!\n");
		return false;
	}

	return true;
}

bool proxi_sensor::is_data_ready(bool wait)
{
	bool ret;

	ret =  update_value(wait);
	return ret;
}

bool proxi_sensor::update_name(char *name)
{
	m_name = name;
	return true;
}



bool proxi_sensor::update_version(int version)
{
	m_version = version;
	return true;
}



bool proxi_sensor::update_id(int id)
{
	m_id = id;
	return true;
}

bool proxi_sensor::update_polling_interval(unsigned long val)
{
	AUTOLOCK(m_mutex);

	INFO("Interval is changed from %dms to %dms]", m_polling_interval, val);
	m_polling_interval = val;
	return true;
}



bool proxi_sensor::start(void)
{
	AUTOLOCK(m_mutex);

	if (m_client > 0) {
		m_client++;
		INFO("Proxi sensor fake starting, client cnt = %d\n", m_client);
		return true;
	}

	FILE *fp = NULL;

	fp = fopen(m_enable_resource.c_str(), "w");
	if (!fp) {
		ERR("Failed to open a resource file: %s\n", m_enable_resource.c_str());
		return false;
	}

	if (fprintf(fp, "%d", 1) < 0) {
		ERR("Failed to enable proxi_sensor\n");
		fclose(fp);
		return false;
	}

	if(fp)
		fclose(fp);

	m_client = 1;
	INFO("Proxi sensor real starting, client cnt = %d\n", m_client);

/*
 * To prevent sf_get_data() getting wrong state before proximity state is updated,
 * sleep for 10ms for state to be updated
 *
*/
	usleep(10000);
	return true;
}



bool proxi_sensor::stop(void)
{

	AUTOLOCK(m_mutex);

	if (m_client > 1) {
		m_client--;
		INFO("Proxi sensor fake stopping, client cnt = %d\n", m_client);
		return true;
	}

	FILE *fp;

	fp = fopen(m_enable_resource.c_str(), "w");
	if (!fp) {
		ERR("Failed to open a resource file : %s\n", m_enable_resource.c_str());
		return false;
	}

	if (fprintf(fp, "%d", 0) < 0) {
		ERR("Failed to disable proxi_sensor\n");
		fclose(fp);
		return false;
	}
	fclose(fp);

	m_client = 0;
	INFO("Proxi sensor real stopping, client cnt = %d\n", m_client);
	return true;
}

int proxi_sensor::get_sensor_type(void)
{
	return m_sensor_type;
}


bool proxi_sensor::check_hw_node(void)
{

	string name_node;
	string hw_name;

	DIR *main_dir = NULL;
	struct dirent *dir_entry = NULL;
	bool find_node = false;

	INFO("======================start check_hw_node=============================\n");

	main_dir = opendir("/sys/class/sensors/");
	if (!main_dir) {
		ERR("Directory open failed to collect data\n");
		return false;
	}

	while ((!find_node) && (dir_entry = readdir(main_dir))) {
		if ((strncasecmp(dir_entry->d_name ,".",1 ) != 0) && (strncasecmp(dir_entry->d_name ,"..",2 ) != 0) && (dir_entry->d_ino != 0)) {

			name_node = string("/sys/class/sensors/") + string(dir_entry->d_name) + string("/name");

			ifstream infile(name_node.c_str());

			if (!infile)
				continue;

			infile >> hw_name;

			if (CConfig::getInstance().is_supported(SENSOR_TYPE_PROXI, hw_name) == true) {
				m_name = m_model_id = hw_name;
				INFO("m_model_id = %s\n",m_model_id.c_str());
				find_node = true;
				break;
			}
		}
	}

	closedir(main_dir);

	if(find_node)
	{
		main_dir = opendir("/sys/class/input/");
		if (!main_dir) {
			ERR("Directory open failed to collect data\n");
			return false;
		}

		find_node = false;

		while ((!find_node) && (dir_entry = readdir(main_dir))) {
			if (strncasecmp(dir_entry->d_name ,"input", 5) == 0) {
				name_node = string("/sys/class/input/") + string(dir_entry->d_name) + string("/name");
				ifstream infile(name_node.c_str());

				if (!infile)
					continue;

				infile >> hw_name;

				if (hw_name == string(INPUT_NAME)) {
					INFO("name_node = %s\n",name_node.c_str());
					DBG("Find H/W  for proxi_sensor\n");

					find_node = true;

					string dir_name;
					dir_name = string(dir_entry->d_name);
					unsigned found = dir_name.find_first_not_of("input");

					m_resource = string("/dev/input/event") + dir_name.substr(found);
					m_enable_resource = string("/sys/class/input/") + string(dir_entry->d_name) + string("/enable");

					break;
				}
			}
		}
		closedir(main_dir);
	}

	if (find_node) {
		INFO("m_resource = %s\n", m_resource.c_str());
		INFO("m_enable_resource = %s\n", m_enable_resource.c_str());
	}

	return find_node;

}



long proxi_sensor::set_cmd(int type , int property , long input_value)
{
	long value = -1;

	if (type == m_sensor_type) {
		switch (property) {
			default :
				ERR("Invalid property_cmd = %d\n", property);
				break;
		}
	}
	else {
		ERR("Invalid sensor_type = %d\n", type);
	}

	return value;
}

int proxi_sensor::get_property(unsigned int property_level , void *property_data)
{
	base_property_struct *return_property;
	return_property = (base_property_struct *)property_data;

	if (property_level == PROXIMITY_BASE_DATA_SET) {
		return_property->sensor_unit_idx = IDX_UNIT_STATE_ON_OFF;
		return_property->sensor_min_range = 0;
		return_property->sensor_max_range = 1;
		return_property->sensor_resolution = 1;
		snprintf(return_property->sensor_name,   sizeof(return_property->sensor_name),"%s", m_chip_name.c_str());
		snprintf(return_property->sensor_vendor, sizeof(return_property->sensor_vendor),"%s", m_vendor.c_str());
		return 0;
	} else if (property_level == PROXIMITY_DISTANCE_DATA_SET) {
		return_property->sensor_unit_idx = IDX_UNIT_CENTIMETER;
		return_property->sensor_min_range = 0;
		return_property->sensor_max_range = 10;
		return_property->sensor_resolution = 1;
		return 0;
	} else {
		ERR("Doesnot support property_level : %d\n",property_level);
		return -1;
	}

	return -1;
}

int proxi_sensor::get_struct_value(unsigned int struct_type , void *struct_values)
{
	base_data_struct *return_struct_value = NULL;
	return_struct_value = (base_data_struct *)struct_values;

	if (return_struct_value == NULL) {
		ERR("return struct_value is NULL\n");
		return -1;
	}

	AUTOLOCK(m_value_mutex);

	if (struct_type == PROXIMITY_BASE_DATA_SET) {
		return_struct_value->data_accuracy = ACCURACY_UNDEFINED;
		return_struct_value->data_unit_idx = IDX_UNIT_STATE_ON_OFF;
		return_struct_value->time_stamp = m_fired_time;
		return_struct_value->values_num = 1;
		return_struct_value->values[0] = m_state;
		return 0;


	} else if (struct_type == PROXIMITY_DISTANCE_DATA_SET) {
		return_struct_value->data_accuracy = ACCURACY_UNDEFINED;
		return_struct_value->data_unit_idx = IDX_UNIT_STATE_ON_OFF;
		return_struct_value->time_stamp = m_fired_time ;
		return_struct_value->values_num = 1;
		return_struct_value->values[0] = (float)((PROXIMITY_STATE_NEAR - m_state) * 5);
		return 0;
	}

	return -1;
}




cmodule *module_init(void *win, void *egl)
{
	proxi_sensor *sample;

	try {
		sample = new proxi_sensor();
	} catch (int ErrNo) {
		ERR("proxi_sensor class create fail , errno : %d , errstr : %s\n",ErrNo, strerror(ErrNo));
		return NULL;
	}

	return (cmodule*)sample;
}



void module_exit(cmodule *inst)
{
	proxi_sensor *sample = (proxi_sensor*)inst;
	delete sample;
}
