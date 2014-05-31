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
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/poll.h>

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

#include <pedo-sensor.h>
#include <sys/ioctl.h>
#include <fstream>


using std::ifstream;


#define SENSOR_TYPE_PEDO		"PEDO"
#define ELEMENT_NAME			"NAME"
#define ELEMENT_VENDOR			"VENDOR"

#define ATTR_VALUE				"value"

#define VALUE_YES				"yes"
#define VALUE_NO				"no"

#define PORT_COUNT 1

#define MS_TO_US 1000llu

/*
  *  Pedometer is a virtual sensor, which is implemented using accelerometer sensor.
*/
#define INPUT_NAME	"accelerometer_sensor"

const char *pedo_sensor::m_port[] = {"step_cnt"};

pedo_sensor::pedo_sensor()
: m_sensor_type(ID_PEDO)
, m_id(0x0000045A)
, m_version(1)
, m_polling_interval(POLL_10HZ_MS)
, m_step_cnt(0)
, m_fired_time(0)
, m_client(0)
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

	INFO("m_chip_name = %s\n", m_chip_name.c_str());

	INFO("pedo_sensor is created!\n");
}



pedo_sensor::~pedo_sensor()
{
	INFO("pedo_sensor is destroyed!\n");
}


using config::CConfig;

bool pedo_sensor::get_config(const string& element,const string& attr, string& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}

	return CConfig::getInstance().get(SENSOR_TYPE_PEDO, m_model_id, element, attr, value);
}

bool pedo_sensor::get_config(const string& element,const string& attr, double& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}

	return CConfig::getInstance().get(SENSOR_TYPE_PEDO, m_model_id, element, attr, value);

}

bool pedo_sensor::get_config(const string& element,const string& attr, long& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}
	return CConfig::getInstance().get(SENSOR_TYPE_PEDO, m_model_id, element, attr, value);
}

const char *pedo_sensor::name(void)
{
	return m_name.c_str();
}

int pedo_sensor::version(void)
{
	return m_version;
}

int pedo_sensor::id(void)
{
	return m_id;
}

bool pedo_sensor::update_value(bool wait)
{

	FILE *fp_step_cnt;;
	unsigned int step_cnt;

	if (wait)
		usleep(m_polling_interval * MS_TO_US);

	fp_step_cnt = fopen(m_resource.c_str(), "r");

	if (!fp_step_cnt) {
		ERR("Can't open:%s, err:%s\n", m_resource.c_str(), strerror(errno));
		return false;
	}

	if (fscanf(fp_step_cnt, "%u", &step_cnt) < 1) {
		ERR("No value in %s", m_resource.c_str());
		fclose(fp_step_cnt);
		return false;
	}

	fclose(fp_step_cnt);

	AUTOLOCK(m_value_mutex);
	m_step_cnt = step_cnt;
	m_fired_time = cmodule::get_timestamp();

	return true;
}

bool pedo_sensor::is_data_ready(bool wait)
{
	bool ret;
	ret = update_value(wait);
	return ret;
}



bool pedo_sensor::update_name(char *name)
{
	m_name = name;
	return true;
}



bool pedo_sensor::update_version(int version)
{
	m_version = version;
	return true;
}



bool pedo_sensor::update_id(int id)
{
	m_id = id;
	return true;
}

bool pedo_sensor::update_polling_interval(unsigned long val)
{
	INFO("Interval is changed from %dms to %dms]", m_polling_interval, val);
	m_polling_interval = val;
	return true;
}


bool pedo_sensor::enable_resource(string &resource_node, bool enable)
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


bool pedo_sensor::is_resource_enabled(string &resource_node)
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


bool pedo_sensor::start(void)
{
	AUTOLOCK(m_mutex);

	if (m_client > 0) {
		m_client++;
		INFO("Pedo sensor fake starting, client cnt = %d\n", m_client);
		return true;
	}

	enable_resource(m_enable_resource, true);

	m_client = 1;
	m_fired_time = 0;
	INFO("Pedo sensor real starting, client cnt = %d\n", m_client);
	return true;
}

bool pedo_sensor::stop(void)
{
	AUTOLOCK(m_mutex);

	if (m_client > 1) {
		m_client--;
		INFO("Pedo sensor fake stopping, client cnt = %d\n", m_client);
		return true;
	}

	m_client = 0;
	enable_resource(m_enable_resource, false);
	INFO("Pedo sensor real stopping, client cnt = %d\n", m_client);

	return true;
}

int pedo_sensor::get_sensor_type(void)
{
	return m_sensor_type;
}

long pedo_sensor::set_cmd(int type , int property , long input_value)
{
	long value = -1;
	ERR("Cannot support any cmd\n");
	return value;
}

int pedo_sensor::get_property(unsigned int property_level , void *property_data)
{
	if (property_level == PEDOMETER_BASE_DATA_SET) {
		base_property_struct *return_property;
		return_property = (base_property_struct *)property_data;
		return_property->sensor_unit_idx = IDX_UNIT_STEP;
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

int pedo_sensor::get_struct_value(unsigned int struct_type, void *struct_values)
{
	if (struct_type == PEDOMETER_BASE_DATA_SET) {
		is_data_ready(false);

		base_data_struct *return_struct_value = NULL;
		return_struct_value = (base_data_struct *)struct_values;
		if (return_struct_value) {
			return_struct_value->data_accuracy = ACCURACY_GOOD;
			return_struct_value->data_unit_idx = IDX_UNIT_STEP;
			return_struct_value->time_stamp = m_fired_time ;
			return_struct_value->values_num = PORT_COUNT;
			return_struct_value->values[0] = m_step_cnt;
			return 0;
		} else {
			ERR("return struct_value point error\n");
		}
	} else {
		ERR("Does not support type , struct_type : %d \n", struct_type);
	}
	return -1;
}

bool pedo_sensor::check_hw_node(void)
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

			if (CConfig::getInstance().is_supported(SENSOR_TYPE_PEDO, hw_name) == true) {
				m_name = m_model_id = hw_name;
				INFO("m_model_id = %s\n", m_model_id.c_str());
				find_node = true;
				break;
			}
		}
	}

	closedir(main_dir);

	if(find_node == true) {
		main_dir = opendir("/sys/class/input/");
		if (!main_dir) {
			ERR("Directory open failed to collect data\n");
			return -1;
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
					INFO("name_node = %s\n", name_node.c_str());
					DBG("Find H/W for pedo_sensor\n");

					find_node = true;

					m_resource = string("/sys/class/input/") + string(dir_entry->d_name) + string("/pedo_count");
					m_enable_resource = string("/sys/class/input/") + string(dir_entry->d_name) + string("/pedo_enable");
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
	pedo_sensor *sample;

	try {
		sample = new pedo_sensor();
	} catch (int ErrNo) {
		ERR("pedo_sensor class create fail , errno : %d , errstr : %s\n",ErrNo, strerror(ErrNo));
		return NULL;
	}

	return (cmodule*)sample;
}



void module_exit(cmodule *inst)
{
	pedo_sensor *sample = (pedo_sensor*)inst;
	delete sample;
}
