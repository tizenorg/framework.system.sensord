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

#include <accel-sensor.h>
#include <sys/ioctl.h>
#include <fstream>

using std::ifstream;


#define ACC_DEV_NODE "/dev/accelerometer"

#define WAKEUP_NODE			"/sys/class/sensors/accelerometer_sensor/reactive_alert"
#define CALIBRATION_NODE	"/sys/class/sensors/accelerometer_sensor/calibration"
#define CALIBRATION_FILE	"/csa/sensor/accel_cal_data"
#define CALIBRATION_DIR		"/csa/sensor"

#define REACTIVE_ALERT_OFF	0
#define REACTIVE_ALERT_ON	1

#define GRAVITY 9.80665
#define G_TO_MG 1000
#define RAW_DATA_TO_G_UNIT(X) (((float)(X))/((float)G_TO_MG))
#define RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT(X) (GRAVITY * (RAW_DATA_TO_G_UNIT(X)))

#define MIN_RANGE(RES) (-((2 << (RES))/2))
#define MAX_RANGE(RES) (((2 << (RES))/2)-1)

#define SENSOR_TYPE_ACCEL		"ACCEL"
#define ELEMENT_NAME			"NAME"
#define ELEMENT_VENDOR			"VENDOR"
#define ELEMENT_RAW_DATA_UNIT	"RAW_DATA_UNIT"
#define ELEMENT_RESOLUTION		"RESOLUTION"

#define ATTR_VALUE				"value"

#define INPUT_NAME	"accelerometer_sensor"

static const int64_t accel_delay_ns[] = {
	744047LL , /* 1344Hz */
	2500000LL , /*  400Hz */
	5000000LL , /*  200Hz */
	10000000LL , /*  100Hz */
	20000000LL , /*   50Hz */
	40000000LL , /*   25Hz */
	100000000LL , /*   10Hz */
	1000000000LL , /*    1Hz */
};

struct accel_data {
	int16_t x;
	int16_t y;
	int16_t z;
};

accel_sensor::accel_sensor()
: m_id(0x0000045A)
, m_version(1)
, m_polling_interval(POLL_1HZ_MS)
, m_x(-1)
, m_y(-1)
, m_z(-1)
, m_fired_time(0)
, m_client(0)
, m_wakeup_client(0)
, m_sensor_type(ID_ACCEL)
, m_node_handle(-1)
, m_sensorhub_supported(false)
{
	if (check_hw_node() == false) {
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

	INFO("m_chip_name = %s\n",m_chip_name.c_str());

	long resolution;

	if (get_config(ELEMENT_RESOLUTION,ATTR_VALUE, resolution) == false) {
		ERR("[RESOLUTION] is empty\n");
		throw ENXIO;
	}

	m_resolution = (int)resolution;

	INFO("m_resolution = %d\n",m_resolution);

	double raw_data_unit;

	if (get_config(ELEMENT_RAW_DATA_UNIT,ATTR_VALUE, raw_data_unit) == false) {
		ERR("[RAW_DATA_UNIT] is empty\n");
		throw ENXIO;
	}

	m_raw_data_unit = (float)(raw_data_unit);

	if ((m_node_handle = open(m_resource.c_str(), O_RDWR)) < 0) {
		ERR("accel handle open fail for accel processor, error:%s\n", strerror(errno));
		throw ENXIO;
	}

	int clockId = CLOCK_MONOTONIC;
	if (ioctl(m_node_handle, EVIOCSCLOCKID, &clockId) != 0) {
		ERR("Fail to set monotonic timestamp for %s", m_resource.c_str());
		throw ENXIO;
	}

	INFO("m_raw_data_unit = %d\n",m_raw_data_unit);
	INFO("accel_sensor is created!\n");
}



accel_sensor::~accel_sensor()
{
	close(m_node_handle);
	m_node_handle = -1;

	INFO("accel_sensor is destroyed!\n");
}


using config::CConfig;

bool accel_sensor::get_config(const string& element,const string& attr, string& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}

	return CConfig::getInstance().get(SENSOR_TYPE_ACCEL,m_model_id,element,attr,value);
}

bool accel_sensor::get_config(const string& element,const string& attr, double& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}

	return CConfig::getInstance().get(SENSOR_TYPE_ACCEL,m_model_id,element,attr,value);

}

bool accel_sensor::get_config(const string& element,const string& attr, long& value)
{
	if (m_model_id.empty()) {
		ERR("model_id is empty\n");
		return false;

	}
	return CConfig::getInstance().get(SENSOR_TYPE_ACCEL,m_model_id,element,attr,value);
}



const char *accel_sensor::name(void)
{
	return m_name.c_str();
}



int accel_sensor::version(void)
{
	return m_version;
}



int accel_sensor::id(void)
{
	return m_id;
}

bool accel_sensor::update_value(bool wait)
{
	const int TIMEOUT = 1;
	int accel_raw[3] = {0,};
	bool x,y,z;
	int read_input_cnt = 0;
	const int INPUT_MAX_BEFORE_SYN = 10;
	unsigned long long fired_time = 0;
	bool syn = false;

	x = y = z = false;

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

		struct input_event accel_input;
		DBG("accel event detection!");

		while ((syn == false) && (read_input_cnt < INPUT_MAX_BEFORE_SYN)) {
			int len = read(m_node_handle, &accel_input, sizeof(accel_input));
			if (len != sizeof(accel_input)) {
				ERR("accel_file read fail, read_len = %d\n",len);
				return false;
			}

			++read_input_cnt;

			if (accel_input.type == EV_REL) {
				switch (accel_input.code) {
					case REL_X:
						accel_raw[0] = (int)accel_input.value;
						x = true;
						break;
					case REL_Y:
						accel_raw[1] = (int)accel_input.value;
						y = true;
						break;
					case REL_Z:
						accel_raw[2] = (int)accel_input.value;
						z = true;
						break;
					default:
						ERR("accel_input event[type = %d, code = %d] is unknown.", accel_input.type, accel_input.code);
						return false;
						break;
				}
			} else if (accel_input.type == EV_SYN) {
				syn = true;
				fired_time = cmodule::get_timestamp(&accel_input.time);
			} else {
				ERR("accel_input event[type = %d, code = %d] is unknown.", accel_input.type, accel_input.code);
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

	if (x)
		m_x =  accel_raw[0];
	if (y)
		m_y =  accel_raw[1];
	if (z)
		m_z =  accel_raw[2];

	m_fired_time = fired_time;

	DBG("m_x = %d, m_y = %d, m_z = %d, time = %lluus", m_x, m_y, m_z, m_fired_time);

	return true;
}





bool accel_sensor::is_data_ready(bool wait)
{
	bool ret;
	ret = update_value(wait);
	return ret;
}

bool accel_sensor::update_name(char *name)
{
	m_name = name;
	return true;
}



bool accel_sensor::update_version(int version)
{
	m_version = version;
	return true;
}



bool accel_sensor::update_id(int id)
{
	m_id = id;
	return true;
}


bool accel_sensor::update_polling_interval(unsigned long val)
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


bool accel_sensor::enable_resource(string &resource_node, bool enable)
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

	if (enable) {
		if (m_sensorhub_supported) {
			status = prev_status | (1 << SENSORHUB_ACCELEROMETER_ENABLE_BIT);
		} else {
			status = 1;
		}
	} else {
		if (m_sensorhub_supported) {
			status = prev_status ^ (1 << SENSORHUB_ACCELEROMETER_ENABLE_BIT);
		} else {
			status = 0;
		}
	}

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


bool accel_sensor::is_resource_enabled(string &resource_node)
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


bool accel_sensor::start(void)
{
	AUTOLOCK(m_mutex);

	if (m_client > 0) {
		m_client++;
		INFO("Accel sensor fake starting, client cnt = %d\n", m_client);
		return true;
	}

	enable_resource(m_enable_resource, true);

	update_polling_interval(m_polling_interval);
	update_polling_interval(m_polling_interval);

	m_client = 1;
	m_fired_time = 0;
	INFO("Accel sensor real starting, client cnt = %d\n", m_client);
	return true;
}

bool accel_sensor::stop(void)
{
	AUTOLOCK(m_mutex);

	if (m_client > 1) {
		m_client--;
		INFO("Accel sensor fake stopping, client cnt = %d\n", m_client);
		return true;
	}

	if (m_wakeup_client == 0) {
		enable_resource(m_enable_resource, false);
	} else {
		INFO("Sensor can't be disabled because of %d clients of wake up", m_wakeup_client); 
	}

	m_client = 0;
	INFO("Accel sensor real stopping, client cnt = %d\n", m_client);
	return true;
}

int accel_sensor::get_sensor_type(void)
{
	return m_sensor_type;
}

int accel_sensor::set_reactive_alert(int input)
{
	FILE *fp;

	fp = fopen(WAKEUP_NODE, "w");

	if (!fp) {
		ERR("Can't open:%s, err:%s\n", WAKEUP_NODE, strerror(errno));
		return -1;
	}

	INFO("set_reactive_alert(%d)\n", input);

	if (input == REACTIVE_ALERT_OFF) {
		fprintf(fp, "%d", REACTIVE_ALERT_OFF);
	} else {
		fprintf(fp, "%d", REACTIVE_ALERT_OFF);
		fflush(fp);
		rewind(fp);
		fprintf(fp, "%d", REACTIVE_ALERT_ON);
		fflush(fp);
	}

	fclose(fp);
	return 0;
}

long accel_sensor::set_cmd(int type , int property , long input_value)
{
	long value = -1;
	FILE *fp;
	AUTOLOCK(m_mutex);

	if (type == m_sensor_type) {
		switch (property) {

			case ACCELEROMETER_PROPERTY_SET_CALIBRATION :
				if ( calibration(CAL_SET) ) {
					INFO("acc_sensor_calibration OK\n");
					value = 0;
				} else {
					ERR("acc_sensor_calibration FAIL\n");
				}
				break;

			case ACCELEROMETER_PROPERTY_CHECK_CALIBRATION_STATUS :
				return (calibration(CAL_CHECK)) ? 0 : -1;

			case ACCELEROMETER_PROPERTY_SET_WAKEUP :
				if (input_value == 0) {
					if (m_wakeup_client <= 0) {
						ERR("m_wakeup_client should not be negative");
						return -1;
					}

					--m_wakeup_client;

					if (m_wakeup_client == 0) {
						set_reactive_alert(REACTIVE_ALERT_OFF);

						if ((m_client == 0) && (is_resource_enabled(m_enable_resource) == true)) {
							enable_resource(m_enable_resource, false);
							INFO("Accel sensor is disabled becauese of both m_client and m_wakeup_client being zero");
						}
					}

				} else {
					++m_wakeup_client;

					if (m_wakeup_client == 1) {
						if (is_resource_enabled(m_enable_resource) == false) {
							enable_resource(m_enable_resource, true);
							INFO("Accel sensor is enabled becauese of the first wakeup client comming up");
						}

						set_reactive_alert(REACTIVE_ALERT_ON);
					}
				}
				return 0;
				break;

			case ACCELEROMETER_PROPERTY_CHECK_WAKEUP_STATUS :
				if (m_wakeup_client > 0)
					return 1;

				return 0;
				break;
			case ACCELEROMETER_PROPERTY_CHECK_WAKEUP_SUPPORTED :
				fp = fopen(WAKEUP_NODE, "r");

				if (!fp) {
					ERR("Failed to open a resource file\n");
					return -1;
				} else {
					fclose(fp);
					return 0;
				}

				break;
			case ACCELEROMETER_PROPERTY_GET_WAKEUP:
				int reactive_alert;
				FILE *fp;

				fp = fopen(WAKEUP_NODE, "r");

				if (!fp) {
					ERR("Can't open:%s, err:%s\n", WAKEUP_NODE, strerror(errno));
					return -1;
				}

				if (fscanf(fp, "%d", &reactive_alert) < 1) {
					ERR("No value in %s", WAKEUP_NODE);
					fclose(fp);
					set_reactive_alert(REACTIVE_ALERT_ON);
					return -1;
				}

				if (reactive_alert == 1) {
					INFO("Reactive alert detected, reactive_alert = %d", reactive_alert);
					fclose(fp);
					set_reactive_alert(REACTIVE_ALERT_ON);
					return 1;
				}

				fclose(fp);
				return 0;

				break;
			default:
				ERR("Invalid property_cmd\n");
				break;
		}
	}
	else {
		ERR("Invalid sensor_type\n");
	}

	return value;

}

int accel_sensor::get_property(unsigned int property_level , void *property_data)
{
	if (property_level == ACCELEROMETER_BASE_DATA_SET) {
		base_property_struct *return_property;
		return_property = (base_property_struct *)property_data;
		return_property->sensor_unit_idx = IDX_UNIT_METRE_PER_SECOND_SQUARED;
		return_property->sensor_min_range = MIN_RANGE(m_resolution)* RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT(m_raw_data_unit);
		return_property->sensor_max_range = MAX_RANGE(m_resolution)* RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT(m_raw_data_unit);
		snprintf(return_property->sensor_name,   sizeof(return_property->sensor_name),"%s", m_chip_name.c_str());
		snprintf(return_property->sensor_vendor, sizeof(return_property->sensor_vendor),"%s", m_vendor.c_str());
		return_property->sensor_resolution = RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT(m_raw_data_unit);
		return 0;

	} else {
		ERR("Doesnot support property_level : %d\n",property_level);
		return -1;
	}

	return -1;
}

int accel_sensor::get_struct_value(unsigned int struct_type , void *struct_values)
{
	const int chance = 3;
	int retry = 0;

	if (struct_type == ACCELEROMETER_BASE_DATA_SET) {
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
			return_struct_value->data_accuracy = ACCURACY_GOOD;
			return_struct_value->data_unit_idx = IDX_UNIT_VENDOR_UNIT;
			return_struct_value->time_stamp = m_fired_time ;
			return_struct_value->values_num = 3;
			return_struct_value->values[0] = m_x;
			return_struct_value->values[1] = m_y;
			return_struct_value->values[2] = m_z;

			return 0;
		} else {
			ERR("return struct_value point error\n");
		}

	} else {
		ERR("Does not support type , struct_type : %d \n",struct_type);
	}
	return -1;
}

bool accel_sensor::calibration(int cmd)
{
	if (cmd == CAL_CHECK) {

		struct calibration_data {
			short x;
			short y;
			short z;
		};

		struct calibration_data cal_data;

		if (access(CALIBRATION_FILE, F_OK) == 0) {

			FILE *fp = NULL;
			fp = fopen(CALIBRATION_FILE, "r");

			if (!fp) {
				ERR("cannot open calibration file");
				return false;
			}

			size_t read_cnt;

			read_cnt = fread(&cal_data, sizeof(cal_data), 1, fp);

			if (read_cnt != 1) {
				ERR("cal_data read fail, read_cnt = %d\n",read_cnt);
				fclose(fp);
				return false;
			}

			fclose(fp);

			INFO("x = [%d] y = [%d] z = [%d]",cal_data.x, cal_data.y, cal_data.z);

			if (cal_data.x == 0 && cal_data.y == 0 && cal_data.z == 0) {
				DBG("cal_data values is zero");
				return false;
			} else
				return true;
		} else {
			INFO("cannot access calibration file");
			return false;
		}
	} else if (cmd == CAL_SET) {
		if (mkdir(CALIBRATION_DIR,0755) != 0)
			INFO("mkdir fail");

		FILE *fp;
		fp = fopen(CALIBRATION_NODE, "w");

		if (!fp) {
			ERR("Failed to open a calibration file\n");
			return false;
		}

		if (fprintf(fp, "%d", cmd) < 0) {
			ERR("Failed to set calibration\n");
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	} else if (cmd == CAL_MKDIR) {
		if (mkdir(CALIBRATION_DIR,0755) != 0) {
			ERR("mkdir fail");
			return false;
		}

		return true;
	}

	ERR("Non supported calibration cmd = %d\n" , cmd);
	return false;
}

bool accel_sensor::is_sensorhub_supported(void)
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

bool accel_sensor::check_hw_node(void)
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

			if (CConfig::getInstance().is_supported(SENSOR_TYPE_ACCEL,hw_name) == true) {
				m_name = m_model_id = hw_name;
				INFO("m_model_id = %s\n",m_model_id.c_str());
				find_node = true;
				calibration(CAL_MKDIR);
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
					DBG("Find H/W  for accel_sensor\n");

					find_node = true;

					string dir_name;
					dir_name = string(dir_entry->d_name);
					unsigned found = dir_name.find_first_not_of("input");

					m_resource = string("/dev/input/event") + dir_name.substr(found);
					if (m_sensorhub_supported) {
						m_enable_resource = string("/sys/class/sensors/ssp_sensor/enable");
					} else
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


cmodule *module_init(void *win, void *egl)
{
	accel_sensor *sample;

	try {
		sample = new accel_sensor();
	} catch (int ErrNo) {
		ERR("accel_sensor class create fail , errno : %d , errstr : %s\n",ErrNo, strerror(ErrNo));
		return NULL;
	}

	return (cmodule*)sample;
}



void module_exit(cmodule *inst)
{
	accel_sensor *sample = (accel_sensor*)inst;
	delete sample;
}
