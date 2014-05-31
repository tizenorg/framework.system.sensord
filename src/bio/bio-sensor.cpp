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
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

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
#include <csensor_module.h>
#include <cconfig.h>

#include <bio-sensor.h>
#include <fstream>

using std::ifstream;

#define SENSOR_TYPE_BIO			"BIO"
#define ELEMENT_NAME 			"NAME"
#define ELEMENT_VENDOR			"VENDOR"
#define ATTR_VALUE 				"value"

#define INPUT_NAME	"hrm_raw_sensor"

bio_sensor::bio_sensor()
: m_id(0x00200002)
, m_version(1)
, m_polling_interval(POLL_1HZ_MS)
, m_fired_time(0)
, m_client(0)
, m_sensor_type(ID_BIO)
, m_node_handle(-1)
, m_sensorhub_supported(false)
{

	memset(m_bio_raw, 0, sizeof(m_bio_raw));

	if (check_hw_node() != 1 ) {
		throw ENXIO;
	}

	if (get_config(ELEMENT_VENDOR,ATTR_VALUE, m_vendor) == false) {
		ERR("[VENDOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_vendor = %s\n", m_vendor.c_str());

	if (get_config(ELEMENT_NAME,ATTR_VALUE, m_chip_name) == false) {
		ERR("[NAME] is empty\n");
		throw ENXIO;
	}

	INFO("m_chip_name = %s\n", m_chip_name.c_str());

	if ((m_node_handle = open(m_resource.c_str(), O_RDWR)) < 0) {
		ERR("Bio handle open fail for Bio processor(handle:%d)\n",m_node_handle);
		throw ENXIO;
	}

	int clockId = CLOCK_MONOTONIC;
	if (ioctl(m_node_handle, EVIOCSCLOCKID, &clockId) != 0) {
		ERR("Fail to set monotonic timestamp for %s", m_resource.c_str());
		throw ENXIO;
	}

	INFO("bio_sensor is created!\n");

}



bio_sensor::~bio_sensor()
{
	close(m_node_handle);
	m_node_handle = -1;

	INFO("bio_sensor is destroyed!\n");
}

using config::CConfig;

bool bio_sensor::get_config(const string& element,const string& attr, string& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}

	return CConfig::getInstance().get(SENSOR_TYPE_BIO,m_model_id,element,attr,value);
}

bool bio_sensor::get_config(const string& element,const string& attr, double& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}

	return CConfig::getInstance().get(SENSOR_TYPE_BIO,m_model_id,element,attr,value);

}

bool bio_sensor::get_config(const string& element,const string& attr, long& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}
	return CConfig::getInstance().get(SENSOR_TYPE_BIO,m_model_id,element,attr,value);
}


const char *bio_sensor::name(void)
{
	return m_name.c_str();
}



int bio_sensor::version(void)
{
	return m_version;
}



int bio_sensor::id(void)
{
	return m_id;
}

bool bio_sensor::update_value(bool wait)
{
	const int TIMEOUT = 1;
	int bio_raw[3] = {0,};
	unsigned long long fired_time = 0;
	int read_input_cnt = 0;
	const int INPUT_MAX_BEFORE_SYN = 10;
	bool syn = false;

	struct timeval tv;
	fd_set readfds,exceptfds;

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
		struct input_event bio_input;
		DBG("bio event detection!");

		while ((syn == false) && (read_input_cnt < INPUT_MAX_BEFORE_SYN)) {
			int len = read(m_node_handle, &bio_input, sizeof(bio_input));
			if (len != sizeof(bio_input)) {
				ERR("bio_file read fail, read_len = %d\n", len);
				return false;
			}

			++read_input_cnt;

			if (bio_input.type == EV_REL) {
				switch (bio_input.code) {
				case REL_X:
					bio_raw[0] = (unsigned int)bio_input.value;
					break;
				case REL_Y:
					bio_raw[1] = (unsigned int)bio_input.value;
					break;
				case REL_Z:
					bio_raw[2] = (unsigned int)bio_input.value;
					break;
				default:
					ERR("bio_input event[type = %d, code = %d] is unknown.", bio_input.type, bio_input.code);
					return false;
					break;
				}
			} else if (bio_input.type == EV_SYN) {
				syn = true;
				fired_time = cmodule::get_timestamp(&bio_input.time);
			} else {
				ERR("bio_input event[type = %d, code = %d] is unknown.", bio_input.type, bio_input.code);
				return false;
			}
		}
	} else {
		ERR("select nothing to read!!!\n");
		return false;
	}

	if (syn == false) {
		ERR("EV_SYN didn't come until %d inputs had come", read_input_cnt);
		return false;
	}

	AUTOLOCK(m_value_mutex);

	m_bio_raw[0] = bio_raw[0] - 1;
	m_bio_raw[1] = bio_raw[1] - 1;
	m_bio_raw[2] = bio_raw[2] - 1;
	m_fired_time = fired_time;

	return true;
}



bool bio_sensor::is_data_ready(bool wait)
{
	bool ret;
	ret =  update_value(wait);
	return ret;
}

bool bio_sensor::update_name(char *name)
{
	m_name = name;
	return true;
}



bool bio_sensor::update_version(int version)
{
	m_version = version;
	return true;
}



bool bio_sensor::update_id(int id)
{
	m_id = id;
	return true;
}

bool bio_sensor::update_polling_interval(unsigned long val)
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

bool bio_sensor::enable_resource(string &resource_node, bool enable)
{
	int prev_status, status;

	FILE *fp = NULL;

	fp = fopen(resource_node.c_str(), "r");

	if (!fp) {
		ERR("Fail to open a resource file: %s\n", resource_node.c_str());
		return false;
	}

	if (fscanf(fp, "%d", &prev_status) < 0) {
		ERR("Failed to get data from %s\n", resource_node.c_str());
		fclose(fp);
		return false;
	}

	fclose(fp);

	INFO("pre_status: %d", prev_status);

	if (enable) {
		if (m_sensorhub_supported) {
			status = prev_status | (1 << SENSORHUB_BIO_ENABLE_BIT);
		} else {
			status = 1;
		}
	} else {
		if (m_sensorhub_supported) {
			status = prev_status ^ (1 << SENSORHUB_BIO_ENABLE_BIT);
		} else {
			status = 0;
		}
	}
	INFO("status: %d", status);

	fp = fopen(resource_node.c_str(), "w");
	if (!fp) {
		ERR("Failed to open a resource file: %s\n", resource_node.c_str());
		return false;
	}

	if (fprintf(fp, "%d", status) < 0) {
		ERR("Failed to enable a resource file: %s\n", resource_node.c_str());
		fclose(fp);
		return false;
	}

	if (fp)
		fclose(fp);

	return true;

}


bool bio_sensor::start(void)
{
	AUTOLOCK(m_mutex);

	if (m_client > 0) {
		m_client++;
		INFO("Bio sensor fake starting, client cnt = %d\n", m_client);
		return true;
	}

	enable_resource(m_enable_resource, true);

	update_polling_interval(m_polling_interval);
	update_polling_interval(m_polling_interval);

	m_client = 1;
	m_fired_time = 0;
	INFO("Bio sensor real starting, client cnt = %d\n", m_client);
	return true;
}



bool bio_sensor::stop(void)
{

	AUTOLOCK(m_mutex);

	INFO("bio_sensor client cnt = %d\n", m_client);

	if (m_client > 1) {
		m_client--;
		INFO("Bio sensor fake stopping, client cnt = %d\n", m_client);
		return true;
	}

	enable_resource(m_enable_resource, false);

	m_client = 0;
	INFO("Bio sensor real stopping, client cnt = %d\n", m_client);
	return true;
}

int bio_sensor::get_sensor_type(void)
{
	return m_sensor_type;
}

long bio_sensor::set_cmd(int type , int property , long input_value)
{
	long value = -1;

	ERR("Cannot support any cmd\n");

	return value;
}

int bio_sensor::get_property(unsigned int property_level , void *property_data)
{
	DBG("bio_sensor called get_property , with property_level : 0x%x", property_level);

	if (property_level == BIO_BASE_DATA_SET) {
		base_property_struct *return_property;
		return_property = (base_property_struct *)property_data;
		return_property->sensor_unit_idx = IDX_UNDEFINED_UNIT;
		return_property->sensor_min_range = 0.0f;
		return_property->sensor_max_range = 1.0f;
		snprintf(return_property->sensor_name, sizeof(return_property->sensor_name),"%s", m_chip_name.c_str());
		snprintf(return_property->sensor_vendor, sizeof(return_property->sensor_vendor),"%s", m_vendor.c_str());
		return_property->sensor_resolution = 1.0f;
		return 0;
	} else {
		ERR("Doesnot support property_level : %d\n",property_level);
		return -1;
	}
	return -1;
}

int bio_sensor::get_struct_value(unsigned int struct_type , void *struct_values)
{
	const int chance = 3;
	int retry = 0;

	if (struct_type == BIO_BASE_DATA_SET) {
		while ((m_fired_time == 0) && (retry++ < chance)) {
			INFO("Try usleep for getting a valid BASE DATA value");
			usleep(m_polling_interval * 1000llu);
		}
		if (m_fired_time == 0) {
			ERR("get_struct_value failed");
			return -1;
		}

		base_data_struct *return_struct_value = NULL;
		return_struct_value = (base_data_struct *)struct_values;
		if ( return_struct_value ) {
			AUTOLOCK(m_value_mutex);
			return_struct_value->data_accuracy = ACCURACY_NORMAL;
			return_struct_value->data_unit_idx = IDX_UNDEFINED_UNIT;
			return_struct_value->time_stamp = m_fired_time ;
			return_struct_value->values_num = 3;
			return_struct_value->values[0] = m_bio_raw[0];
			return_struct_value->values[1] = m_bio_raw[1];
			return_struct_value->values[2] = m_bio_raw[2];

			return 0;
		} else {
			ERR("return struct_value point error\n");
		}
	} else {
		ERR("Does not support type , struct_type : %d \n",struct_type);
	}
	return -1;
}

bool bio_sensor::is_sensorhub_supported(void)
{
	DIR *main_dir = NULL;

	main_dir = opendir("/sys/class/sensors/ssp_sensor");

	if (!main_dir) {
		INFO("Sensor Hub is not supported\n");
		return false;
	}

	INFO("It supports sensor hub");

	closedir(main_dir);

	return true;
}

bool bio_sensor::check_hw_node(void)
{

	string name_node;
	string hw_name;

	DIR *main_dir = NULL;
	struct dirent *dir_entry = NULL;
	bool find_node = false;

	INFO("======================start check_hw_node=============================\n");

	m_sensorhub_supported = is_sensorhub_supported();

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

			if (CConfig::getInstance().is_supported(SENSOR_TYPE_BIO,hw_name) == true) {
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
			if (strncasecmp(dir_entry->d_name ,"input",5 ) == 0) {
				name_node = string("/sys/class/input/") + string(dir_entry->d_name) + string("/name");
				ifstream infile(name_node.c_str());

				if (!infile)
					continue;

				infile >> hw_name;

				if (hw_name == string(INPUT_NAME)) {
					INFO("name_node = %s\n",name_node.c_str());
					DBG("Find H/W  for bio_sensor\n");

					find_node = true;

					string dir_name;
					dir_name = string(dir_entry->d_name);
					unsigned found = dir_name.find_first_not_of("input");

					m_resource = string("/dev/input/event") + dir_name.substr(found);

					if (m_sensorhub_supported) {
						m_enable_resource = string("/sys/class/sensors/ssp_sensor/enable");
						m_polling_resource = string("/sys/class/sensors/ssp_sensor/hrm_poll_delay");
					} else {
						m_enable_resource = string("/sys/class/input/") + string(dir_entry->d_name) + string("/enable");
						m_polling_resource = string("/sys/class/input/") + string(dir_entry->d_name) + string("/poll_delay");
					}
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



cmodule *module_init(void *win, void *egl)
{
	bio_sensor *sample;

	try {
		sample = new bio_sensor();
	} catch (int ErrNo) {
		ERR("bio_sensor class create fail , errno : %d , errstr : %s\n",ErrNo, strerror(ErrNo));
		return NULL;
	}

	return (cmodule*)sample;
}



void module_exit(cmodule *inst)
{
	bio_sensor *sample = (bio_sensor*)inst;
	delete sample;
}



//! End of a file
