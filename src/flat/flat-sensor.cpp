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

#include <flat-sensor.h>
#include <errno.h>


#include <cconfig.h>
#include <fstream>


using std::ifstream;


#define SENSOR_TYPE_FLAT		"FLAT"
#define ELEMENT_NAME			"NAME"
#define ELEMENT_VENDOR			"VENDOR"

#define ATTR_VALUE				"value"

#define VALUE_YES				"yes"
#define VALUE_NO				"no"

#define PORT_COUNT 1

#define MS_TO_US 1000llu

/*
  *  Flatmeter is a virtual sensor, which is implemented using accelerometer sensor.
*/
#define INPUT_NAME	"flat_sensor"

const char *flat_sensor::m_port[] = {"state"};

flat_sensor::flat_sensor()
: m_sensor_type(ID_FLAT)
, m_id(0x0000045A)
, m_version(1)
, m_polling_interval(200)
, m_fired_time(0)
, m_client(0)
, m_node_handle(-1)
, m_flat_state(FLAT_UNKNOWN)
{
	if (check_hw_node() == false) {
		ERR("check_hw_node() fail\n");
		throw ENXIO;
	}

	if (get_config(ELEMENT_VENDOR, ATTR_VALUE, m_vendor) == false) {
		ERR("[VENDOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_vendor = %s\n", m_vendor.c_str());

	if (get_config(ELEMENT_NAME, ATTR_VALUE, m_chip_name) == false) {
		ERR("[NAME] is empty\n");
		throw ENXIO;
	}

	if ((m_node_handle = open(m_resource.c_str(), O_RDWR)) < 0) {
		ERR("Flat handle open fail for Flat processor(handle:%d)\n",m_node_handle);
		throw ENXIO;
	}

	int clockId = CLOCK_MONOTONIC;
	if (ioctl(m_node_handle, EVIOCSCLOCKID, &clockId) != 0) {
		ERR("Fail to set monotonic timestamp for %s", m_resource.c_str());
		throw ENXIO;
	}

	INFO("m_chip_name = %s\n", m_chip_name.c_str());

	INFO("flat_sensor is created!\n");
}



flat_sensor::~flat_sensor()
{
	close(m_node_handle);
	m_node_handle = -1;

	INFO("flat_sensor is destroyed!\n");
}


using config::CConfig;

bool flat_sensor::get_config(const string& element,const string& attr, string& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}

	return CConfig::getInstance().get(SENSOR_TYPE_FLAT, m_model_id, element, attr, value);
}

bool flat_sensor::get_config(const string& element,const string& attr, double& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}

	return CConfig::getInstance().get(SENSOR_TYPE_FLAT, m_model_id, element, attr, value);

}

bool flat_sensor::get_config(const string& element,const string& attr, long& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}
	return CConfig::getInstance().get(SENSOR_TYPE_FLAT, m_model_id, element, attr, value);
}

const char *flat_sensor::name(void)
{
	return m_name.c_str();
}

int flat_sensor::version(void)
{
	return m_version;
}

int flat_sensor::id(void)
{
	return m_id;
}

bool flat_sensor::update_value(bool wait)
{

	struct timeval tv;
	fd_set readfds,exceptfds;
	const int TIMEOUT = 1;

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
		DBG("select timeout: %d seconds elapsed.\n",tv.tv_sec);
		return true;
	}

	if (FD_ISSET(m_node_handle, &exceptfds)) {
			ERR("select exception occurred!\n");
			return false;
	}

	if (FD_ISSET(m_node_handle, &readfds)) {
		struct input_event flat_event;
		DBG("flat event detection!");

		int len;
		len = read(m_node_handle, &flat_event, sizeof(flat_event));

		if (len == -1) {
			DBG("read(m_node_handle) is error:%s.\n", strerror(errno));
			return false;
		}

		DBG("read event,  len : %d , type : %x , code : %x , value : %x", len, flat_event.type, flat_event.code, flat_event.value);
		if ((flat_event.type == EV_REL) && (flat_event.code == REL_MISC)) {
			AUTOLOCK(m_value_mutex);
			if (flat_event.value == FLAT_FACE_UP) {
				INFO("FLAT_FACE_UP state occured\n");
				m_flat_state = FLAT_FACE_UP;
			} else if (flat_event.value == FLAT_FACE_DOWN) {
				INFO("FLAT_FACE_DOWN state occured\n");
				m_flat_state = FLAT_FACE_DOWN;
			} else if (flat_event.value == FLAT_NONE) {
				INFO("FLAT_NONE state occured\n");
				m_flat_state = FLAT_NONE;
			} else {
				ERR("FLAT state Unknown: %d\n",flat_event.value);
				return false;
			}
			m_fired_time = cmodule::get_timestamp(&flat_event.time);
		} else {
			return false;
		}
	} else {
		ERR("select nothing to read!!!\n");
		return false;
	}

	return true;
}


bool flat_sensor::is_data_ready(bool wait)
{
	bool ret;
	ret = update_value(wait);
	return ret;
}

bool flat_sensor::update_name(char *name)
{
	m_name = name;
	return true;
}



bool flat_sensor::update_version(int version)
{
	m_version = version;
	return true;
}



bool flat_sensor::update_id(int id)
{
	m_id = id;
	return true;
}

bool flat_sensor::update_polling_interval(unsigned long val)
{
	DBG("Update polling interval %lu\n", val);
	AUTOLOCK(m_mutex);
	m_polling_interval = (unsigned long long)val * 1000llu;
	return true;
}


bool flat_sensor::enable_resource(string &resource_node, bool enable)
{
	FILE *fp = NULL;

	fp = fopen(resource_node.c_str(), "w");
	if (!fp) {
		ERR("Failed to open a resource file: %s\n", resource_node.c_str());
		return false;
	}

	if (fprintf(fp, "%d", enable ? 1 : 0) < 0) {
		ERR("Failed to enable a resource file: %s\n", resource_node.c_str());
		fclose(fp);
		return false;
	}

	if (fp)
		fclose(fp);

	return true;

}


bool flat_sensor::is_resource_enabled(string &resource_node)
{
	int status;

	FILE *fp = NULL;

	fp = fopen(resource_node.c_str(), "r");

	if (!fp) {
		ERR("Fail to open a resource file: %s\n", resource_node.c_str());
		return false;
}

	if (fscanf(fp, "%d", &status) < 0) {
		ERR("Failed to get data from %s\n", resource_node.c_str());
		fclose(fp);
		return false;
	}

	fclose(fp);

	if (status == 1)
		return true;

	return false;
}


bool flat_sensor::start(void)
{
	AUTOLOCK(m_mutex);

	if (m_client > 0) {
		m_client++;
		INFO("Flat sensor fake starting, client cnt = %d\n", m_client);
		return true;
	}

	enable_resource(m_enable_resource, true);

	m_client = 1;
	m_fired_time = 0;
	INFO("Flat sensor real starting, client cnt = %d\n", m_client);
	return true;
}

bool flat_sensor::stop(void)
{
	AUTOLOCK(m_mutex);

	if (m_client > 1) {
		m_client--;
		INFO("Flat sensor fake stopping, client cnt = %d\n", m_client);
		return true;
	}

	m_client = 0;
	enable_resource(m_enable_resource, false);
	INFO("Flat sensor real stopping, client cnt = %d\n", m_client);

	return true;
}

int flat_sensor::get_sensor_type(void)
{
	return m_sensor_type;
}

long flat_sensor::set_cmd(int type , int property , long input_value)
{
	long value = -1;
	ERR("Cannot support any cmd\n");
	return value;
}

int flat_sensor::get_property(unsigned int property_level , void *property_data)
{
	if (property_level == FLAT_BASE_DATA_SET) {
		base_property_struct *return_property;
		return_property = (base_property_struct *)property_data;
		return_property->sensor_unit_idx = IDX_UNDEFINED_UNIT;
		return_property->sensor_min_range = 1;
		return_property->sensor_max_range = 1;
		snprintf(return_property->sensor_name,   sizeof(return_property->sensor_name),"%s", m_chip_name.c_str());
		snprintf(return_property->sensor_vendor, sizeof(return_property->sensor_vendor),"%s", m_vendor.c_str());
		return_property->sensor_resolution = 1;
		return 0;

	} else {
		ERR("Doesnot support property_level : %d\n",property_level);
		return -1;
	}

	return -1;
}

int flat_sensor::get_struct_value(unsigned int struct_type, void *struct_values)
{
	if (struct_type == FLAT_BASE_DATA_SET) {
		is_data_ready(false);

		base_data_struct *return_struct_value = NULL;
		return_struct_value = (base_data_struct *)struct_values;
		if (return_struct_value) {
			return_struct_value->data_accuracy = ACCURACY_GOOD;
			return_struct_value->data_unit_idx = IDX_UNDEFINED_UNIT;
			return_struct_value->time_stamp = m_fired_time ;
			return_struct_value->values_num = PORT_COUNT;
			return_struct_value->values[0] = m_flat_state;
			return 0;
		} else {
			ERR("return struct_value point error\n");
		}
	} else {
		ERR("Does not support type , struct_type : %d \n", struct_type);
	}
	return -1;
}

bool flat_sensor::check_hw_node(void)
{
	string name_node;
	string hw_name;

	DIR *main_dir = NULL;
	struct dirent *dir_entry = NULL;
	int find_node = false;

	INFO("======================start check_hw_node=============================\n");

	main_dir = opendir("/sys/class/sensors/");
	if (!main_dir) {
		ERR("Directory open failed to collect data\n");
		return false;
	}

	while ((!find_node) && (dir_entry = readdir(main_dir))) {
		if ((strncasecmp(dir_entry->d_name ,".",1 ) != 0) && (strncasecmp(dir_entry->d_name ,"..", 2) != 0) && (dir_entry->d_ino != 0)) {

			name_node = string("/sys/class/sensors/") + string(dir_entry->d_name) + string("/name");

			ifstream infile(name_node.c_str());

			if (!infile) {
				continue;
			}

			infile >> hw_name;

			if (CConfig::getInstance().is_supported(SENSOR_TYPE_FLAT, hw_name) == true) {
				m_name = m_model_id = hw_name;
				INFO("m_model_id = %s\n", m_model_id.c_str());
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
					DBG("Find H/W  for flat_sensor\n");

					find_node = true;

					string dir_name;
					dir_name = string(dir_entry->d_name);
					unsigned found = dir_name.find_first_not_of("input");

					m_resource = string("/dev/input/event") + dir_name.substr(found);
					m_enable_resource = string("/sys/class/input/") + string(dir_entry->d_name) + string("/lcd_pos_enable");
					break;
				}
			}
		}
		closedir(main_dir);
	}

	if (find_node == true) {
		INFO("m_resource = %s\n", m_resource.c_str());
		INFO("m_enable_resource = %s\n", m_enable_resource.c_str());
	}

	return find_node;

}


cmodule *module_init(void *win, void *egl)
{
	flat_sensor *sample;

	try {
		sample = new flat_sensor();
	} catch (int ErrNo) {
		ERR("flat_sensor class create fail , errno : %d , errstr : %s\n",ErrNo, strerror(ErrNo));
		return NULL;
	}

	return (cmodule*)sample;
}



void module_exit(cmodule *inst)
{
	flat_sensor *sample = (flat_sensor*)inst;
	delete sample;
}
