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
#include <cmutex.h>
#include <clist.h>
#include <cmodule.h>
#include <cpacket.h>
#include <cworker.h>
#include <csock.h>
#include <sf_common.h>
#include <linux/input.h>
#include <dirent.h>
#include <time.h>

#include <csensor_module.h>

#include <light-sensor.h>
#include <cconfig.h>
#include <fstream>

using std::ifstream;

#define SENSOR_TYPE_LIGHT		"LIGHT"
#define ELEMENT_NAME			"NAME"
#define ELEMENT_VENDOR			"VENDOR"
#define ELEMENT_RAW_DATA_UNIT	"RAW_DATA_UNIT"
#define ELEMENT_RESOLUTION		"RESOLUTION"

#define ATTR_VALUE				"value"

#define BIAS	1

#define INVALID_VALUE	-1

#define MAX_STR_LENGTH	256

#define INPUT_NAME	"light_sensor"

const int MAX_LIGHT_LEVEL = 13;
const char *light_sensor::m_port[] = {"adc","level"};
const int light_level[MAX_LIGHT_LEVEL] = {0, 1, 165, 288, 497, 869, 1532, 2692, 4692, 8280,21428,65535,137852};

light_sensor::light_sensor()
: m_id(117)
, m_version(1)
, m_polling_interval(POLL_1HZ_MS)
, m_adc(INVALID_VALUE)
, m_level(INVALID_VALUE)
, m_fired_time(0)
, m_client(0)
, m_node_handle(-1)
, m_sensor_type(ID_LUMINANT)
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

	if ((m_node_handle = open(m_resource.c_str(),O_RDWR)) < 0) {
		ERR("Light sensor handle open fail for Light processor(handle:%d)\n", m_node_handle);
		throw ENXIO;
	}

	int clockId = CLOCK_MONOTONIC;
	if (ioctl(m_node_handle, EVIOCSCLOCKID, &clockId) != 0) {
		ERR("Fail to set monotonic timestamp for %s", m_resource.c_str());
		throw ENXIO;
	}

	INFO("m_chip_name = %s\n",m_chip_name.c_str());
	INFO("light_sensor is created!\n");
}



light_sensor::~light_sensor()
{
	close(m_node_handle);
	m_node_handle = -1;

	INFO("light_sensor is destroyed!\n");

}

using config::CConfig;

bool light_sensor::get_config(const string& element,const string& attr, string& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}

	return CConfig::getInstance().get(SENSOR_TYPE_LIGHT,m_model_id,element,attr,value);
}

bool light_sensor::get_config(const string& element,const string& attr, double& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}

	return CConfig::getInstance().get(SENSOR_TYPE_LIGHT,m_model_id,element,attr,value);

}

bool light_sensor::get_config(const string& element,const string& attr, long& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}
	return CConfig::getInstance().get(SENSOR_TYPE_LIGHT,m_model_id,element,attr,value);
}


const char *light_sensor::name(void)
{
	return m_name.c_str();
}



int light_sensor::version(void)
{
	return m_version;
}



int light_sensor::id(void)
{
	return m_id;
}



bool light_sensor::update_value(bool wait)
{
	const int TIMEOUT = 1;
	struct timeval tv;
	fd_set readfds,exceptfds;

	int i = 0;
	int adc = -1;
	int level = -1;

	FD_ZERO(&readfds);
	FD_ZERO(&exceptfds);

	FD_SET(m_node_handle, &readfds);
	FD_SET(m_node_handle, &exceptfds);

	if (wait) {
		tv.tv_sec = TIMEOUT;
		tv.tv_usec = 0;
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
		struct input_event light_event;
		DBG("light event detection!");

		int len;
		len = read(m_node_handle, &light_event, sizeof(light_event));

		if (len == -1) {
			DBG("read(m_node_handle) is error:%s.\n", strerror(errno));
			return false;
		}

		if (light_event.type == EV_ABS && light_event.code == ABS_MISC) {
			adc = light_event.value;
		} else if (light_event.type == EV_REL && light_event.code == REL_MISC) {
			adc = light_event.value - BIAS;
		} else {
			return false;
		}

		DBG("read event,  len : %d , type : %x , code : %x , value : %x", len , light_event.type , light_event.code , light_event.value);

		for (i = 0 ; i < MAX_LIGHT_LEVEL - 1; i++) {
			if (adc >= light_level[i] && adc < light_level[i + 1]) {
				level = i;
				break;
			}
		}
		DBG("update_value, adc : %d, level : %d", adc, level);

		AUTOLOCK(m_value_mutex);
		m_level = level;
		m_adc = adc;
		m_fired_time = cmodule::get_timestamp(&light_event.time);
	} else {
		ERR("select nothing to read!!!\n");
		return false;
	}
	return true;
}



bool light_sensor::is_data_ready(bool wait)
{
	bool ret;

	ret =  update_value(wait);
	return ret;
}

bool light_sensor::update_name(char *name)
{
	m_name = name;
	return true;
}



bool light_sensor::update_version(int version)
{
	m_version = version;
	return true;
}



bool light_sensor::update_id(int id)
{
	m_id = id;
	return true;
}

bool light_sensor::update_polling_interval(unsigned long val)
{
	unsigned long long polling_interval_ns;

	AUTOLOCK(m_mutex);

	polling_interval_ns = ((unsigned long long)(val) * 1000llu * 1000llu);

	FILE *fp = NULL;

	fp = fopen(m_polling_resource.c_str(), "w");
	if (!fp) {
		ERR("Failed to open a resource file: %s\n", m_polling_resource.c_str());
		return false;
	}

	if (fprintf(fp, "%llu", polling_interval_ns) < 0) {
		ERR("Failed to set data %llu\n", polling_interval_ns);
		fclose(fp);
		return false;
	}

	if (fp)
		fclose(fp);

	INFO("Interval is changed from %dms to %dms]", m_polling_interval, val);
	m_polling_interval = val;


	return true;
}



bool light_sensor::start(void)
{
	AUTOLOCK(m_mutex);

	if (m_client > 0) {
		m_client++;
		INFO("Light sensor fake starting, client cnt = %d\n", m_client);
		return true;
	}

	update_polling_interval(m_polling_interval);

	FILE *fp = NULL;
	fp = fopen(m_enable_resource.c_str(), "w");
	if (!fp) {
		ERR("Failed to open a resource file\n");
		return false;
	}

	if (fprintf(fp, "%d", 1) < 0) {
		ERR("Failed to enable light_sensor\n");
		fclose(fp);
		return false;
	}
	if (fp)
		fclose(fp);

	m_client = 1;
	m_fired_time = 0;
	INFO("Light sensor real starting, client cnt = %d\n", m_client);
	return true;
}



bool light_sensor::stop(void)
{
	AUTOLOCK(m_mutex);

	INFO("light_sensor client cnt = %d\n", m_client);

	if (m_client > 1) {
		DBG("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Stopping\n");
		m_client--;
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
	m_level = m_adc = INVALID_VALUE;
	return true;
}

int light_sensor::get_sensor_type(void)
{
	return m_sensor_type;
}


bool light_sensor::check_hw_node(void)
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

			if (!infile) {
				continue;
			}

			infile >> hw_name;

			if (CConfig::getInstance().is_supported(SENSOR_TYPE_LIGHT, hw_name) == true) {
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
					DBG("Find H/W  for light_sensor\n");

					find_node = true;

					string dir_name;
					dir_name = string(dir_entry->d_name);
					unsigned found = dir_name.find_first_not_of("input");

					m_resource = string("/dev/input/event") + dir_name.substr(found);
					m_enable_resource = string("/sys/class/input/") + string(dir_entry->d_name) + string("/enable");
					m_polling_resource = string("/sys/class/input/") + string(dir_entry->d_name) + string("/poll_delay");

					break;
				}
			}
		}
		closedir(main_dir);
	}

	if (find_node) {
		INFO("m_resource = %s\n", m_resource.c_str());
		INFO("m_enable_resource = %s\n", m_enable_resource.c_str());
		INFO("m_polling_resource = %s\n", m_polling_resource.c_str());
	}

	return find_node;

}

long light_sensor::set_cmd(int type , int property , long input_value)
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

int light_sensor::get_property(unsigned int property_level , void *property_data)
{
	base_property_struct *return_property;
	return_property = (base_property_struct *)property_data;

	if (property_level == LIGHT_BASE_DATA_SET) {
		return_property->sensor_unit_idx = IDX_UNIT_LEVEL_1_TO_10;
		return_property->sensor_min_range = 1.;
		return_property->sensor_max_range = 10.;
		return_property->sensor_resolution = 1.;
		snprintf(return_property->sensor_name,   sizeof(return_property->sensor_name),"%s", m_chip_name.c_str());
		snprintf(return_property->sensor_vendor, sizeof(return_property->sensor_vendor),"%s", m_vendor.c_str());
		return 0;

	} else  if (property_level == LIGHT_LUX_DATA_SET) {
		return_property->sensor_unit_idx = IDX_UNIT_LUX;
		return_property->sensor_min_range = 0;
		return_property->sensor_max_range = 65525;
		return_property->sensor_resolution = 1;
		return 0;
	} else {
		ERR("Doesnot support property_level : %d\n",property_level);
		return -1;
	}

	return -1;
}

int light_sensor::get_struct_value(unsigned int struct_type , void *struct_values)
{
	const int chance = 3;
	int retry = 0;

	base_data_struct *return_struct_value = NULL;
	return_struct_value = (base_data_struct *)struct_values;

	if (return_struct_value == NULL) {
		ERR("return struct_value is NULL\n");
		return -1;
	}

	if (struct_type == LIGHT_BASE_DATA_SET) {
		return_struct_value->data_accuracy = ACCURACY_UNDEFINED;
		return_struct_value->data_unit_idx = IDX_UNIT_LEVEL_1_TO_10;
		return_struct_value->values_num = 1;
		while ((m_fired_time == 0) && (retry++ < chance)) {
			INFO("Try usleep for getting a valid BASE DATA value");
			usleep(m_polling_interval * 1000llu);
		}
		if (m_fired_time == 0) {
			ERR("get_struct_value failed");
			return -1;
		}

		AUTOLOCK(m_value_mutex);
		return_struct_value->values[0] = (float)m_level;
		return_struct_value->time_stamp = m_fired_time ;
		return 0;
	} else if (struct_type == LIGHT_LUX_DATA_SET) {
		return_struct_value->data_accuracy = ACCURACY_UNDEFINED;
		return_struct_value->data_unit_idx = IDX_UNIT_LUX;
		return_struct_value->values_num = 1;
		while ((m_fired_time == 0) && (retry++ < chance)) {
			INFO("Try usleep for getting a valid LUX DATA value");
			usleep(m_polling_interval * 1000llu);
		}
		if (m_fired_time == 0) {
			ERR("get_struct_value failed");
			return -1;
		}

		AUTOLOCK(m_value_mutex);
		return_struct_value->values[0] = (float)m_adc;
		return_struct_value->time_stamp = m_fired_time ;
		return 0;
	} else {
		ERR("Does not support type , struct_type : %d \n",struct_type);
	}

	return -1;
}


cmodule *module_init(void *win, void *egl)
{
	light_sensor *sample;

	try {
		sample = new light_sensor();
	} catch (int ErrNo) {
		ERR("light_sensor class create fail , errno : %d , errstr : %s\n",ErrNo, strerror(ErrNo));
		return NULL;
	}

	return (cmodule*)sample;
}



void module_exit(cmodule *inst)
{
	light_sensor *sample = (light_sensor*)inst;
	delete sample;
}



//! End of a file
