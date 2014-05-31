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
#include <accel-processor.h>

#include <vconf.h>

#include <algorithm>



using std::bind1st;
using std::mem_fun;


#define ROTATION_0 0
#define ROTATION_90 90
#define ROTATION_180 180
#define ROTATION_270 270
#define ROTATION_360 360
#define ROTATION_THD 45
#define RADIAN_VALUE 57.29747
#define GRAVITY 9.80665
#define G_TO_MG 1000

#define RAW_DATA_TO_G_UNIT(X) (((float)(X))/((float)G_TO_MG))
#define RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT(X) (GRAVITY * (RAW_DATA_TO_G_UNIT(X)))

#define LOW_FILTER_ALPHA 0.8
#define LOW_FILTER_ALPHA_R 0.2
#define MS_TO_US 1000

#define HORIZON_STRING_LENGTH 100
#define HORIZON_SEQ 30
#define SMART_ROTATION_LIB "/usr/lib/sensor_framework/libsmart_detection.so"

#define NAME_SUFFIX "accel_processor"

#define ROTATION_CHECK_INTERVAL	200

#define ROTATION_RULE_CNT	4
#define TILT_MIN		30
const char *accel_processor::LCD_TYPE_NODE = "/sys/class/graphics/fb0/modes";


struct rotation_rule {
	int tilt;
	int angle;
};

struct rotation_rule rot_rule[ROTATION_RULE_CNT] = {
	{40, 80} ,
	{50, 70} ,
	{60, 65} ,
	{90, 60} ,
};


accel_processor::accel_processor()
: m_sensor(NULL)
, m_x(-1)
, m_y(-1)
, m_z(-1)
, m_gravity_x(-1)
, m_gravity_y(-1)
, m_gravity_z(-1)
, m_cur_rotation(0)
, m_rotation(0)
, m_valid_rotation(0)
, m_version(1)
, m_id(0x04BE)
, m_client(0)
, m_rotation_time(0)
, m_polling_interval(POLL_1HZ_MS)
, m_curr_window_count(0)
, m_set_horizon_count(0)
, m_smart_rotation_handle(NULL)
, m_smart_rotation_state(false)
, m_smart_rotation_progressing(false)
, m_smart_rotation_wait_cnt(0)
, m_get_smart_detection(NULL)
, m_pop_sync_rotation_mode(false)
, m_rotation_check_interval(ROTATION_CHECK_INTERVAL)
{
	rotation_mode[0].rotation = ROTATION_UNKNOWN;
	rotation_mode[0].rm[0] = ROTATION_UNKNOWN;
	rotation_mode[0].rm[1] = ROTATION_UNKNOWN;

	rotation_mode[1].rotation = ROTATION_HEAD_ROTATE_90;
	rotation_mode[1].rm[0] = ROTATION_LANDSCAPE_LEFT;
	rotation_mode[1].rm[1] = ROTATION_PORTRAIT_BTM;

	rotation_mode[2].rotation = ROTATION_HEAD_ROTATE_0;
	rotation_mode[2].rm[0] = ROTATION_PORTRAIT_TOP;
	rotation_mode[2].rm[1] = ROTATION_LANDSCAPE_LEFT;

	rotation_mode[3].rotation = ROTATION_HEAD_ROTATE_180;
	rotation_mode[3].rm[0] = ROTATION_PORTRAIT_BTM;
	rotation_mode[3].rm[1] = ROTATION_LANDSCAPE_RIGHT;

	rotation_mode[4].rotation = ROTATION_HEAD_ROTATE_270;
	rotation_mode[4].rm[0] = ROTATION_LANDSCAPE_RIGHT;
	rotation_mode[4].rm[1] = ROTATION_PORTRAIT_TOP;

	m_lcd_type = check_lcd_type();

	vector<unsigned int> supported_events = {
		ACCELEROMETER_EVENT_ROTATION_CHECK,
		ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME,
		ACCELEROMETER_EVENT_SET_HORIZON,
		ACCELEROMETER_EVENT_SET_WAKEUP,
		ACCELEROMETER_EVENT_ORIENTATION_DATA_REPORT_ON_TIME,
		ACCELEROMETER_EVENT_LINEAR_ACCELERATION_DATA_REPORT_ON_TIME,
		ACCELEROMETER_EVENT_GRAVITY_DATA_REPORT_ON_TIME,
	};

	for_each(supported_events.begin(), supported_events.end(),
		bind1st(mem_fun(&cprocessor_module::register_supported_event), this));

	m_smart_rotation_state = is_smart_rotation_on();

	load_smart_rotation();

	cprocessor_module::set_main(working, stopped, this);

	INFO("accel_processor is created!\n");
}

accel_processor::~accel_processor()
{
	INFO("accel_processor is destroyed!\n");
}

bool accel_processor::add_input(csensor_module *sensor)
{
	m_sensor = sensor;
	m_name = string(NAME_SUFFIX);

	base_property_struct property_data;

	if (sensor->get_property(ACCELEROMETER_BASE_DATA_SET, &property_data) != 0) {
		ERR("sensor->get_property() is failed!\n");
		return false;
	}

	m_raw_data_unit = property_data.sensor_resolution / GRAVITY * G_TO_MG;
	return true;
}


bool accel_processor::add_input(cprocessor_module *processor)
{
    return true;
}

const char *accel_processor::name(void)
{
	return m_name.c_str();
}



int accel_processor::id(void)
{
	return m_id;
}



int accel_processor::version(void)
{
	return m_version;
}



int accel_processor::get_processor_type(void)
{
	if (m_sensor)
	        return m_sensor->get_sensor_type();

        return ID_ACCEL;
}


bool accel_processor::update_name(char *name)
{
	m_name = name;
	return true;
}



bool accel_processor::update_id(int id)
{
	m_id = id;
	return true;
}



bool accel_processor::update_version(int version)
{
	m_version = version;
	return true;
}



cprocessor_module *accel_processor::create_new(void)
{
	return (cprocessor_module*)this;
}



void accel_processor::destroy(cprocessor_module *module)
{
	bool bstate = false;

	bstate = cmodule::del_from_list((cmodule *)module);

	if (!bstate) {
		ERR("Destory and del_from_list fail");
		delete (accel_processor *)module;
		return ;
	}
}

void *accel_processor::stopped(void *inst)
{
	return (void*)NULL;
}


void *accel_processor::working(void *inst)
{
	accel_processor *processor = (accel_processor*)inst;
	return (void*)(processor->process_event());
}


bool accel_processor::is_smart_rotation_on(void)
{
	return false;
}

void accel_processor::reset_smart_rotation(void)
{
	m_smart_rotation_progressing = false;
	m_smart_rotation_wait_cnt = 0;
}

void accel_processor::reset_auto_rotation(void)
{
	int i;

	for (i = 0 ; i < MAX_WINDOW_NUM ; i++)
		m_windowing[i] = 0;

	m_curr_window_count = 0;
	m_rotation_check_remained_time = m_rotation_check_interval;
}

bool accel_processor::is_auto_rotation_time(void)
{
	m_rotation_check_remained_time -= m_polling_interval;
	if (m_rotation_check_remained_time <= 0) {
		m_rotation_check_remained_time = m_rotation_check_interval;
		return true;
	}

	return false;
}

bool accel_processor::load_smart_rotation(void)
{
	AUTOLOCK(m_mutex);

	m_smart_rotation_handle = dlopen(SMART_ROTATION_LIB, RTLD_LAZY);

	if (!m_smart_rotation_handle) {
		ERR("dlopen(%s) error\n", SMART_ROTATION_LIB);
		return false;
	}

	m_get_smart_detection = (get_smart_detection_t) dlsym(m_smart_rotation_handle, "get_smart_detection");
	if (!m_get_smart_detection) {
		ERR("dlsym(\"get_smart_detection\") fail\n");
		dlclose(m_smart_rotation_handle);
		m_smart_rotation_handle = NULL;
		return false;
	}
	return true;
}

bool accel_processor::unload_smart_rotation(void)
{
	AUTOLOCK(m_mutex);

	if (m_smart_rotation_handle != NULL) {
		dlclose(m_smart_rotation_handle);
		m_smart_rotation_handle = NULL;
	}

	m_get_smart_detection = NULL;

	return true;
}

void accel_processor::smart_rotation_enable_cb(keynode_t *node, void *data)
{
	accel_processor *processor = (accel_processor*)data;

	cmutex &mutex = processor->m_mutex;
	AUTOLOCK(mutex);

	processor->m_smart_rotation_state = processor->is_smart_rotation_on();

	INFO("smart rotation state == [%s]", processor->m_smart_rotation_state ? "true" : "false");
}


void accel_processor::smart_rotation_callback(int result, void *user_data)
{
	sensor_event_t event;

	accel_processor* processor = (accel_processor*) user_data;

	cmutex &client_info_mutex = processor->m_client_info_mutex;
	AUTOLOCK(client_info_mutex);

	cmutex &mutex = processor->m_mutex;
	AUTOLOCK(mutex);

	if(result < 0) {
		_D("Smart rotation failed, rotate screen: %d", processor->m_cur_rotation);
		processor->m_rotation_time = get_timestamp();

		if (processor->get_client_cnt(ACCELEROMETER_EVENT_ROTATION_CHECK)) {
			event.timestamp = processor->get_timestamp();
			event.event_type = ACCELEROMETER_EVENT_ROTATION_CHECK;
			event.values_num = 1;
			event.values[0] = processor->m_cur_rotation;
			processor->push(event);
		}
	} else {
		_D("Smart rotation succeeded");
	}

	processor->reset_smart_rotation();
}


void accel_processor::pop_sync_rotation_enable_cb(keynode_t *node, void *data)
{
	int val;
	int state;

	accel_processor *processor = (accel_processor*)data;

	val = vconf_keynode_get_bool(node);

	INFO("vconf_notify_key_changed node name = [%s] value = [%d]", vconf_keynode_get_name(node), val);

	cmutex &mutex = processor->m_mutex;
	AUTOLOCK(mutex);

	processor->m_pop_sync_rotation_mode  = (val == 0) ? true : false;

	if (processor->m_pop_sync_rotation_mode) {
		processor->reset_smart_rotation();
	} else {
		processor->reset_auto_rotation();
	}
}

void accel_processor::pop_sync_rotation_cb(keynode_t *node, void *data)
{
	int rotation;

	accel_processor *processor = (accel_processor*)data;

	rotation = vconf_keynode_get_int(node);

	DBG("vconf_notify_key_changed node name = [%s] value = [%d]", vconf_keynode_get_name(node), rotation);

	processor->manual_rotate(rotation);
}

void accel_processor::manual_rotate(int rotation)
{
	AUTOLOCK(m_mutex);

	if (m_rotation != rotation) {
		sensor_event_t event;

		reset_auto_rotation();
		m_rotation_time = get_timestamp();
		m_cur_rotation = rotation;
		set_rotation(rotation);

		INFO("Manual rotation event occurred, rotation value = %d", rotation);
		event.event_type = ACCELEROMETER_EVENT_ROTATION_CHECK;
		event.values_num = 1;
		event.values[0] = rotation;
		push(event);
	}
}

void accel_processor::set_rotation(int rotation)
{
	m_rotation = rotation;

	if (m_rotation != ROTATION_UNKNOWN)
		m_valid_rotation = m_rotation;

	DBG("m_rotation: %d, m_valid_rotation: %d", m_rotation, m_valid_rotation);
}


int accel_processor::process_event(void)
{
	float x, y, z;
	sensor_event_t event;

	if (!m_sensor) {
		ERR("Sensor is not added\n");
		return cworker::STOPPED;
	}

	{ /* scope for the lock */
		AUTOLOCK(m_client_info_mutex);

		if (get_client_cnt(ACCELEROMETER_EVENT_SET_WAKEUP)) {
			if (m_sensor->set_cmd(ID_ACCEL, ACCELEROMETER_PROPERTY_GET_WAKEUP, 0) == 1) {
				INFO("Reactive alert is detected!\n");
				event.event_type = ACCELEROMETER_EVENT_SET_WAKEUP;
				event.timestamp = get_timestamp();
				push(event);
			}
		}
	}

	if (m_sensor->is_data_ready(true) == false) {
		return cworker::STARTED;
	}


	base_data_struct raw_data;
	m_sensor->get_struct_value(ACCELEROMETER_BASE_DATA_SET , &raw_data);

	x = raw_data.values[0];
	y = raw_data.values[1];
	z = raw_data.values[2];
	event.timestamp = raw_data.time_stamp;

	AUTOLOCK(m_client_info_mutex);
	AUTOLOCK(m_mutex);

	if (get_client_cnt(ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME)) {
		base_data_struct base_data;
		raw_to_base(&raw_data, &base_data);

		event.event_type = ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME;
		base_data_to_sensor_event(&base_data, &event);
		push(event);
	}


	if (get_client_cnt(ACCELEROMETER_EVENT_ORIENTATION_DATA_REPORT_ON_TIME)) {
		base_data_struct orientation_data;
		raw_to_orientation(&raw_data, &orientation_data);

		event.event_type = ACCELEROMETER_EVENT_ORIENTATION_DATA_REPORT_ON_TIME;
		base_data_to_sensor_event(&orientation_data, &event);
		push(event);
	}

	if (get_client_cnt(ACCELEROMETER_EVENT_LINEAR_ACCELERATION_DATA_REPORT_ON_TIME)) {
		base_data_struct linear_data;
		raw_to_linear(&raw_data, &linear_data);

		event.event_type = ACCELEROMETER_EVENT_LINEAR_ACCELERATION_DATA_REPORT_ON_TIME;
		base_data_to_sensor_event(&linear_data, &event);
		push(event);
	}

	if (get_client_cnt(ACCELEROMETER_EVENT_GRAVITY_DATA_REPORT_ON_TIME)) {
		base_data_struct gravity_data;
		raw_to_gravity(&raw_data, &gravity_data);

		event.event_type = ACCELEROMETER_EVENT_GRAVITY_DATA_REPORT_ON_TIME;
		base_data_to_sensor_event(&gravity_data,&event);
		push(event);
	}

	if (!m_pop_sync_rotation_mode && get_client_cnt(ACCELEROMETER_EVENT_ROTATION_CHECK)) {
		if (is_auto_rotation_time()){
			int rotation_event = get_rotation_event(x, y, z);

			if (rotation_event != -1) {
				INFO("Rotation event occurred, rotation value = %d", rotation_event);
				event.event_type = ACCELEROMETER_EVENT_ROTATION_CHECK;
				event.values_num = 1;
				event.values[0] = rotation_event;
				push(event);
			}
		}
	}

	if (get_client_cnt(ACCELEROMETER_EVENT_SET_HORIZON)) {
		m_set_horizon_count++;

		if (m_set_horizon_count > HORIZON_SEQ) {
			m_sensor->set_cmd(ID_ACCEL, ACCELEROMETER_PROPERTY_SET_CALIBRATION, 1);
			m_set_horizon_count = 0;
			event.event_type = ACCELEROMETER_EVENT_SET_HORIZON;
			event.values_num = 0;
			push(event);
		}
	}

	AUTOLOCK(m_value_mutex);
	m_x = x;
	m_y = y;
	m_z = z;

	m_gravity_x = (LOW_FILTER_ALPHA * m_gravity_x) + (LOW_FILTER_ALPHA_R * x);
	m_gravity_y = (LOW_FILTER_ALPHA * m_gravity_y) + (LOW_FILTER_ALPHA_R * y);
	m_gravity_z = (LOW_FILTER_ALPHA * m_gravity_z) + (LOW_FILTER_ALPHA_R * z);

	return cworker::STARTED;


}

bool accel_processor::start(void)
{
	bool ret;

	AUTOLOCK(m_mutex);

	if (m_client > 0) {
		m_client++;
		INFO("%s processor fake starting\n",m_name.c_str());
		return true;
	}

	INFO("%s processor real starting\n",m_name.c_str());

	reset_auto_rotation();
	reset_smart_rotation();

	ret = m_sensor ? m_sensor->start() : false;
	if ( ret != true ) {
		ERR("m_sensor start fail\n");
		return false;
	}

	cprocessor_module::start();
	m_client = 1;
	return ret;
}

bool accel_processor::stop(void)
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
	if (ret != true) {
		ERR("cprocessor_module::stop()\n");
		return false;
	}

	ret = m_sensor ? m_sensor->stop() : false;
	if (ret != true) {
		ERR("m_sensor stop fail\n");
		return false;
	}

	m_client = 0;
	return ret;
}


bool accel_processor::add_client(unsigned int event_type)
{
	AUTOLOCK(m_client_info_mutex);

	if (!is_supported(event_type)) {
		ERR("invaild event type: 0x%x !!", event_type);
		return false;
	}

	cprocessor_module::add_client(event_type);


	switch (event_type) {
	case ACCELEROMETER_EVENT_ROTATION_CHECK:
		if (get_client_cnt(ACCELEROMETER_EVENT_ROTATION_CHECK) == 1) {
			reset_auto_rotation();
		}
		break;
	case ACCELEROMETER_EVENT_SET_WAKEUP:
		if (get_client_cnt(ACCELEROMETER_EVENT_SET_WAKEUP) == 1)
			set_cmd(ID_ACCEL, ACCELEROMETER_PROPERTY_SET_WAKEUP, 1);
		break;
	default:
		break;
	}

	return true;
}

bool accel_processor::delete_client(unsigned int event_type)
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
	case ACCELEROMETER_EVENT_ROTATION_CHECK:
		if (get_client_cnt(ACCELEROMETER_EVENT_ROTATION_CHECK) == 0)
		break;
	case ACCELEROMETER_EVENT_SET_WAKEUP:
		if (get_client_cnt(ACCELEROMETER_EVENT_SET_WAKEUP) == 0)
			set_cmd(ID_ACCEL, ACCELEROMETER_PROPERTY_SET_WAKEUP, 0);
		break;
	default:
		break;
	}

	return true;
}



long accel_processor::set_cmd(int type , int property , long input_value)
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

int accel_processor::get_property(unsigned int property_level , void *property_data )
{
	int result = -1;
	base_property_struct *return_property;
	return_property = (base_property_struct *)property_data;

	if (m_sensor) {
		result = m_sensor->get_property(ACCELEROMETER_BASE_DATA_SET, return_property);
		if (result == 0)	{
			if (property_level == ACCELEROMETER_BASE_DATA_SET) {
				return result;
			} else if (property_level == ACCELEROMETER_ORIENTATION_DATA_SET) {
				return_property->sensor_unit_idx = IDX_UNIT_DEGREE;
				return_property->sensor_min_range = -180;
				return_property->sensor_max_range = 360;
				return_property->sensor_resolution = 1;
            } else if (property_level == ACCELEROMETER_LINEAR_ACCELERATION_DATA_SET) {
				return_property->sensor_unit_idx = IDX_UNIT_METRE_PER_SECOND_SQUARED;
			} else if (property_level == ACCELEROMETER_GRAVITY_DATA_SET) {
				return_property->sensor_unit_idx = IDX_UNIT_METRE_PER_SECOND_SQUARED;
				return_property->sensor_min_range = return_property->sensor_min_range / GRAVITY;
				return_property->sensor_max_range =  return_property->sensor_max_range / GRAVITY;
				return_property->sensor_resolution = return_property->sensor_resolution / GRAVITY;
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

int accel_processor::get_struct_value(unsigned int struct_type , void *struct_values)
{
	int state;
	base_data_struct sensor_struct_data;
	base_data_struct *return_struct_data;

	return_struct_data = (base_data_struct *)struct_values;

	if (struct_type == ACCELEROMETER_ROTATION_DATA_SET) {
		return_struct_data->data_accuracy = ACCURACY_NORMAL;
		return_struct_data->data_unit_idx = IDX_UNDEFINED_UNIT;
		return_struct_data->values_num = 1;
		return_struct_data->values[0] = m_rotation;
		return_struct_data->time_stamp = m_rotation_time;
		return 0;
	}

	state = m_sensor ? m_sensor->get_struct_value(ACCELEROMETER_BASE_DATA_SET , &sensor_struct_data) : -1;
	if (state < 0) {
		ERR("Error , m_sensor get struct_data fail\n");
		return -1;
	}

	if (struct_type == ACCELEROMETER_BASE_DATA_SET)
		raw_to_base(&sensor_struct_data, return_struct_data);
	else if (struct_type == ACCELEROMETER_ORIENTATION_DATA_SET)
		raw_to_orientation(&sensor_struct_data, return_struct_data);
	else if (struct_type == ACCELEROMETER_LINEAR_ACCELERATION_DATA_SET)
		raw_to_linear(&sensor_struct_data, return_struct_data);
	else if (struct_type == ACCELEROMETER_GRAVITY_DATA_SET)
		raw_to_gravity(&sensor_struct_data, return_struct_data);
	else {
		ERR("does not support stuct_type\n");
		return -1;
	}

	return 0;
}

int accel_processor::check_lcd_type(void)
{
	int type=-1;

#if defined(USE_LCD_TYPE_CHECK)

	FILE *fp;
	int x;
	int y;
	int frame;


	fp = fopen(LCD_TYPE_NODE, "r");
	if (!fp) {
		ERR("Failed to open node : %s\n",LCD_TYPE_NODE);
		return type;
	}

	if (fscanf(fp, "U:%dx%dp-%d", &x, &y, &frame) != 3) {
		ERR("Failed to collect data\n");
		fclose(fp);
		return type;
	}
	fclose(fp);

	if (x < y) {
		type = 0;
	} else if (x > y) {
		type = 1;
	} else {
		type = 0 ;
	}
#else
	type = 0;
#endif

	return type;
}


bool accel_processor::update_polling_interval(unsigned long interval)
{
	AUTOLOCK(m_mutex);

	INFO("Polling interval is set to %dms", interval);

	m_polling_interval = interval;
	return m_sensor->update_polling_interval(m_polling_interval);
}

int accel_processor::get_rotation_event(float x, float y, float z)
{
	const int MAX_SMART_ROTATION_WAIT_CNT = 15;
	const int SMART_ROTATION_TYPE = 1;
	int cur_rotation = ROTATION_UNKNOWN;

	double atan_value;
	int acc_theta, acc_pitch;
	double realg;
	bool is_stable = false;
	bool rotation_on = false;

	int tilt, angle;
	int i;

	atan_value = atan2(x,y);
	acc_theta = (int)(atan_value * (RADIAN_VALUE) + 360) % 360;

	realg = (double)sqrt((x * x) + (y * y) + (z * z));

	acc_pitch = ROTATION_90 - abs((int) (asin(z / realg) * RADIAN_VALUE));

	for (i = 0; i < ROTATION_RULE_CNT; ++i) {
		tilt = rot_rule[i].tilt;

		if ((acc_pitch >= TILT_MIN) && (acc_pitch <= tilt)) {
			if ((m_valid_rotation == ROTATION_PORTRAIT_TOP) || (m_valid_rotation == ROTATION_PORTRAIT_BTM))
				angle = rot_rule[i].angle;
			else
				angle = ROTATION_90 - rot_rule[i].angle;

			if ((acc_theta >= ROTATION_360 - angle && acc_theta <= ROTATION_360 - 1) ||
					(acc_theta >= ROTATION_0 && acc_theta <= ROTATION_0 + angle)) {
				cur_rotation = rotation_mode[ROTATION_HEAD_ROTATE_0].rm[m_lcd_type];
			} else if (acc_theta >= ROTATION_0 + angle && acc_theta <= ROTATION_180 - angle) {
				cur_rotation = rotation_mode[ROTATION_HEAD_ROTATE_90].rm[m_lcd_type];
			} else if (acc_theta >= ROTATION_180 - angle && acc_theta <= ROTATION_180 + angle) {
				cur_rotation = rotation_mode[ROTATION_HEAD_ROTATE_180].rm[m_lcd_type];
			} else if (acc_theta >= ROTATION_180 + angle && acc_theta <= ROTATION_360 - angle) {
				cur_rotation = rotation_mode[ROTATION_HEAD_ROTATE_270].rm[m_lcd_type];
			}
			break;
		}
	}

	m_windowing[m_curr_window_count++] = cur_rotation;

	if (m_curr_window_count == MAX_WINDOW_NUM)
		m_curr_window_count = 0;

	for (i = 0; i < MAX_WINDOW_NUM ; i++) {
		if (m_windowing[i] == cur_rotation)
			is_stable = true;
		else {
			is_stable = false;
			break;
		}
	}

	rotation_on = m_rotation ^ cur_rotation;

	_T("rotation_on = %d, is_stable = %d, m_cur_rotation = %d, m_rotation = %d, m_valid_rotation = %d, m_smart_rotation_progressing = %d",
		rotation_on, is_stable, m_cur_rotation, m_rotation, m_valid_rotation, m_smart_rotation_progressing);

	if (rotation_on && is_stable) {
		m_cur_rotation = cur_rotation;
		set_rotation(cur_rotation);
		m_rotation_time = get_timestamp();

		return m_rotation;
	}

	return -1;

}



void accel_processor::raw_to_base(base_data_struct *raw, base_data_struct *base)
{
	base->time_stamp = raw->time_stamp;
	base->data_accuracy = raw->data_accuracy;
	base->data_unit_idx = IDX_UNIT_METRE_PER_SECOND_SQUARED;
	base->values_num = 3;
	base->values[0] = RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT(raw->values[0] * m_raw_data_unit);
	base->values[1] = RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT(raw->values[1] * m_raw_data_unit);
	base->values[2] = RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT(raw->values[2] * m_raw_data_unit);
}


void accel_processor::raw_to_orientation(base_data_struct *raw, base_data_struct *orientation)
{
	orientation->time_stamp = raw->time_stamp;
	orientation->data_accuracy = raw->data_accuracy;
	orientation->data_unit_idx = IDX_UNIT_DEGREE;
	orientation->values_num = 3;
	orientation->values[0] = fmodf((atan2(raw->values[0], raw->values[1]) * RADIAN_VALUE + 360), 360);
	orientation->values[1] = fmodf((atan2(raw->values[1], raw->values[2]) * RADIAN_VALUE), 180);
	orientation->values[2] = fmodf((atan2(raw->values[0], raw->values[2]) * RADIAN_VALUE), 180);

	if (orientation->values[2] > 90)
		orientation->values[2] = 180 - orientation->values[2];
	else if (orientation->values[2] < -90)
		orientation->values[2] = -180 - orientation->values[2];


}
void accel_processor::raw_to_linear(base_data_struct *raw, base_data_struct *linear)
{
	linear->time_stamp = raw->time_stamp;
	linear->data_accuracy = raw->data_accuracy;
	linear->data_unit_idx = IDX_UNIT_METRE_PER_SECOND_SQUARED;
	linear->values_num = 3;

	AUTOLOCK(m_value_mutex);
	linear->values[0] = RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT((raw->values[0] - m_gravity_x) * m_raw_data_unit);
	linear->values[1] = RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT((raw->values[1] - m_gravity_y) * m_raw_data_unit);
	linear->values[2] = RAW_DATA_TO_METRE_PER_SECOND_SQUARED_UNIT((raw->values[2] - m_gravity_z) * m_raw_data_unit);
}

void accel_processor::raw_to_gravity(base_data_struct *raw, base_data_struct *gravity)
{
	gravity->time_stamp = raw->time_stamp;
	gravity->data_accuracy = raw->data_accuracy;
	gravity->data_unit_idx = IDX_UNIT_METRE_PER_SECOND_SQUARED;
	gravity->values_num = 3;
	gravity->values[0] = RAW_DATA_TO_G_UNIT(raw->values[0] * m_raw_data_unit);
	gravity->values[1] = RAW_DATA_TO_G_UNIT(raw->values[1] * m_raw_data_unit);
	gravity->values[2] = RAW_DATA_TO_G_UNIT(raw->values[2] * m_raw_data_unit);
}


cmodule *module_init(void *win, void *egl)
{
	accel_processor *inst;

	try {
		inst = new accel_processor();
	} catch (int ErrNo) {
		ERR("accel_processor class create fail , errno : %d , errstr : %s\n",ErrNo, strerror(ErrNo));
		return NULL;
	}

	return (cmodule*)inst;
}



void module_exit(cmodule *inst)
{
	accel_processor *sample = (accel_processor*)inst;
	delete sample;
}
