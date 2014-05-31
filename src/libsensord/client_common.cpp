/*
 * libslp-sensor
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
#include <client_common.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <map>
using std::map;

struct rotation_event rotation_mode[] =
{
	{ ROTATION_UNKNOWN,	  {	ROTATION_UNKNOWN,		  ROTATION_UNKNOWN		   }},
	{ ROTATION_EVENT_90,  {	ROTATION_LANDSCAPE_LEFT,  ROTATION_PORTRAIT_BTM	   }},
	{ ROTATION_EVENT_0,	  {	ROTATION_PORTRAIT_TOP,	  ROTATION_LANDSCAPE_LEFT  }},
	{ ROTATION_EVENT_180, {	ROTATION_PORTRAIT_BTM,	  ROTATION_LANDSCAPE_RIGHT }},
	{ ROTATION_EVENT_270, {	ROTATION_LANDSCAPE_RIGHT, ROTATION_PORTRAIT_TOP	   }},
};

#define FILL_LOG_ELEMENT(ID, TYPE, CNT, PRINT_PER_CNT) {ID, TYPE, {#TYPE, CNT, PRINT_PER_CNT} }

log_element g_log_elements[] = {
	FILL_LOG_ELEMENT(LOG_ID_SENSOR_TYPE, UNKNOWN_SENSOR, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_SENSOR_TYPE, ACCELEROMETER_SENSOR, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_SENSOR_TYPE, GEOMAGNETIC_SENSOR, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_SENSOR_TYPE, LIGHT_SENSOR, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_SENSOR_TYPE, PROXIMITY_SENSOR, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_SENSOR_TYPE, THERMOMETER_SENSOR, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_SENSOR_TYPE, GYROSCOPE_SENSOR, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_SENSOR_TYPE, BAROMETER_SENSOR, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_SENSOR_TYPE, MOTION_SENSOR, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_SENSOR_TYPE, FUSION_SENSOR, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_SENSOR_TYPE, PEDOMETER_SENSOR, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_SENSOR_TYPE, CONTEXT_SENSOR, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_SENSOR_TYPE, FLAT_SENSOR, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_SENSOR_TYPE, BIO_SENSOR, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_SENSOR_TYPE, BIO_HRM_SENSOR, 0, 1),

	FILL_LOG_ELEMENT(LOG_ID_EVENT, ACCELEROMETER_EVENT_ROTATION_CHECK, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, ACCELEROMETER_EVENT_CALIBRATION_NEEDED, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, ACCELEROMETER_EVENT_SET_HORIZON, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, ACCELEROMETER_EVENT_SET_WAKEUP, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, GEOMAGNETIC_EVENT_CALIBRATION_NEEDED, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, PROXIMITY_EVENT_CHANGE_STATE, 0,1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, LIGHT_EVENT_CHANGE_LEVEL, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, MOTION_ENGINE_EVENT_SNAP, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, MOTION_ENGINE_EVENT_SHAKE, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, MOTION_ENGINE_EVENT_DOUBLETAP, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, MOTION_ENGINE_EVENT_PANNING, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, MOTION_ENGINE_EVENT_TOP_TO_BOTTOM, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, MOTION_ENGINE_EVENT_DIRECT_CALL, 0,1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, MOTION_ENGINE_EVENT_TILT_TO_UNLOCK, 0,1 ),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, MOTION_ENGINE_EVENT_LOCK_EXECUTE_CAMERA, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, MOTION_ENGINE_EVENT_SMART_ALERT, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, MOTION_ENGINE_EVENT_TILT, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, MOTION_ENGINE_EVENT_PANNING_BROWSE, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, MOTION_ENGINE_EVENT_NO_MOVE, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, GEOMAGNETIC_EVENT_RAW_DATA_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, PROXIMITY_EVENT_STATE_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, BAROMETER_EVENT_RAW_DATA_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, LIGHT_EVENT_LEVEL_DATA_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, FUSION_SENSOR_EVENT_RAW_DATA_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, LIGHT_EVENT_LUX_DATA_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, GEOMAGNETIC_EVENT_ATTITUDE_DATA_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, ACCELEROMETER_EVENT_ORIENTATION_DATA_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, BAROMETER_EVENT_TEMPERATURE_DATA_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, PROXIMITY_EVENT_DISTANCE_DATA_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, FUSION_ROTATION_VECTOR_EVENT_DATA_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, ACCELEROMETER_EVENT_LINEAR_ACCELERATION_DATA_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, FUSION_ROTATION_MATRIX_EVENT_DATA_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, BAROMETER_EVENT_ALTITUDE_DATA_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, ACCELEROMETER_EVENT_GRAVITY_DATA_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, PEDOMETER_EVENT_STEP_COUNT, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, CONTEXT_EVENT_REPORT, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, FLAT_EVENT_CHANGE_STATE, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, BIO_EVENT_RAW_DATA_REPORT_ON_TIME, 0, 10),
	FILL_LOG_ELEMENT(LOG_ID_EVENT, BIO_HRM_EVENT_CHANGE_STATE, 0, 10),

	FILL_LOG_ELEMENT(LOG_ID_DATA, ACCELEROMETER_BASE_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, ACCELEROMETER_ORIENTATION_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, ACCELEROMETER_LINEAR_ACCELERATION_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, ACCELEROMETER_GRAVITY_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, ACCELEROMETER_ROTATION_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, GYRO_BASE_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, PROXIMITY_BASE_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, PROXIMITY_DISTANCE_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, BAROMETER_BASE_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, BAROMETER_TEMPERATURE_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, BAROMETER_ALTITUDE_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, GEOMAGNETIC_BASE_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, GEOMAGNETIC_RAW_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, GEOMAGNETIC_ATTITUDE_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, LIGHT_BASE_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, LIGHT_LUX_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, PEDOMETER_BASE_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, CONTEXT_BASE_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, FLAT_BASE_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, BIO_BASE_DATA_SET, 0, 25),
	FILL_LOG_ELEMENT(LOG_ID_DATA, BIO_HRM_BASE_DATA_SET, 0, 25),

	FILL_LOG_ELEMENT(LOG_ID_ROTATE, ROTATION_EVENT_0, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_ROTATE, ROTATION_EVENT_90, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_ROTATE, ROTATION_EVENT_180, 0, 1),
	FILL_LOG_ELEMENT(LOG_ID_ROTATE, ROTATION_EVENT_270, 0, 1),
};

typedef map<unsigned int, log_attr* > log_map;
log_map g_log_maps[LOG_ID_END];

extern void init_client(void);
static void init_log_maps(void);


class initiator
{
public:
	initiator()
	{
		init_log_maps();
		init_client();
	}
} g_initiatior;

static void init_log_maps(void)
{
	int cnt;

	cnt = sizeof(g_log_elements) / sizeof(g_log_elements[0]);

	for (int i = 0; i < cnt; ++i) {
		g_log_maps[g_log_elements[i].id][g_log_elements[i].type] = &g_log_elements[i].log_attr;
	}

}


const char* get_log_element_name(log_id id, unsigned int type)
{
	const char* p_unknown = "UNKNOWN";

	log_map::iterator iter;

	iter = g_log_maps[id].find(type);

	if (iter == g_log_maps[id].end()) {
		INFO("Unknown type value: 0x%x", type);
		return p_unknown;
	}

	return iter->second->name;
}

bool is_sensor_supported(sensor_type sensor)
{
	bool retval = false;
	switch (sensor) {
		case ACCELEROMETER_SENSOR :
		case GEOMAGNETIC_SENSOR :
		case LIGHT_SENSOR:
		case PROXIMITY_SENSOR:
		case MOTION_SENSOR:
		case GYROSCOPE_SENSOR:
		case BAROMETER_SENSOR:
		case FUSION_SENSOR:
		case PEDOMETER_SENSOR:
		case CONTEXT_SENSOR:
		case FLAT_SENSOR:
		case BIO_SENSOR:
		case BIO_HRM_SENSOR:
			retval = true;
			break;
		case THERMOMETER_SENSOR:
		case UNKNOWN_SENSOR:
		default :
			break;
	}
	return retval;
}

const char* get_sensor_name(sensor_type sensor_type)
{
	return get_log_element_name(LOG_ID_SENSOR_TYPE, sensor_type);
}

const char* get_event_name(unsigned int event_type)
{
	return get_log_element_name(LOG_ID_EVENT, event_type);
}


const char* get_data_name(unsigned int data_id)
{
	return get_log_element_name(LOG_ID_DATA, data_id);
}


const char* get_rotate_name(unsigned int rotate_type)
{
	return get_log_element_name(LOG_ID_ROTATE, rotate_type);
}

bool is_sensorhub_event(unsigned int event_type)
{
	switch (event_type) {
	case CONTEXT_EVENT_REPORT:
		return true;
		break;
	}

	return false;
}


bool is_one_shot_event(unsigned int event_type)
{
	switch (event_type) {
	case MOTION_ENGINE_EVENT_SMART_ALERT:
	case ACCELEROMETER_EVENT_SET_WAKEUP:
		return true;
		break;
	}

	return false;
}

bool is_ontime_event(unsigned int event_type)
{
	switch (event_type ) {
	case ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME:
	case ACCELEROMETER_EVENT_ORIENTATION_DATA_REPORT_ON_TIME:
	case ACCELEROMETER_EVENT_LINEAR_ACCELERATION_DATA_REPORT_ON_TIME:
	case ACCELEROMETER_EVENT_GRAVITY_DATA_REPORT_ON_TIME:
	case GEOMAGNETIC_EVENT_ATTITUDE_DATA_REPORT_ON_TIME:
	case PROXIMITY_EVENT_STATE_REPORT_ON_TIME:
	case GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME:
	case BAROMETER_EVENT_RAW_DATA_REPORT_ON_TIME:
	case BAROMETER_EVENT_TEMPERATURE_DATA_REPORT_ON_TIME:
	case BAROMETER_EVENT_ALTITUDE_DATA_REPORT_ON_TIME:
	case LIGHT_EVENT_LEVEL_DATA_REPORT_ON_TIME:
	case GEOMAGNETIC_EVENT_RAW_DATA_REPORT_ON_TIME:
	case LIGHT_EVENT_LUX_DATA_REPORT_ON_TIME:
	case PROXIMITY_EVENT_DISTANCE_DATA_REPORT_ON_TIME:
	case FUSION_SENSOR_EVENT_RAW_DATA_REPORT_ON_TIME:
	case FUSION_ROTATION_VECTOR_EVENT_DATA_REPORT_ON_TIME:
	case FUSION_ROTATION_MATRIX_EVENT_DATA_REPORT_ON_TIME:
	case BIO_EVENT_RAW_DATA_REPORT_ON_TIME:
		return true;
		break;
	}

	return false;

}

bool is_panning_event(unsigned int event_type)
{
	switch (event_type) {
	case MOTION_ENGINE_EVENT_PANNING:
	case MOTION_ENGINE_EVENT_TILT:
	case MOTION_ENGINE_EVENT_PANNING_BROWSE:
		return true;
		break;
	}

	return false;
}

bool is_single_state_event(unsigned int event_type)
{
	switch (event_type) {
	case ACCELEROMETER_EVENT_SET_WAKEUP:
	case ACCELEROMETER_EVENT_ROTATION_CHECK:
	case ACCELEROMETER_EVENT_SET_HORIZON:
	case GEOMAGNETIC_EVENT_CALIBRATION_NEEDED:
	case LIGHT_EVENT_CHANGE_LEVEL:
	case PROXIMITY_EVENT_CHANGE_STATE:
	case MOTION_ENGINE_EVENT_SNAP:
	case MOTION_ENGINE_EVENT_SHAKE:
	case MOTION_ENGINE_EVENT_DOUBLETAP:
	case MOTION_ENGINE_EVENT_TOP_TO_BOTTOM:
	case MOTION_ENGINE_EVENT_DIRECT_CALL:
	case MOTION_ENGINE_EVENT_TILT_TO_UNLOCK:
	case MOTION_ENGINE_EVENT_LOCK_EXECUTE_CAMERA:
	case MOTION_ENGINE_EVENT_SMART_ALERT:
	case MOTION_ENGINE_EVENT_NO_MOVE:
	case PEDOMETER_EVENT_STEP_COUNT:
	case FLAT_EVENT_CHANGE_STATE:
		return true;
		break;
	}

	return false;
}

unsigned int get_calibration_event_type(unsigned int event_type)
{
	sensor_type sensor;

	const int SENSOR_TYPE_SHIFT = 16;
	sensor = (sensor_type)(event_type >> SENSOR_TYPE_SHIFT);

	switch (sensor) {
	case GEOMAGNETIC_SENSOR:
		return GEOMAGNETIC_EVENT_CALIBRATION_NEEDED;
	default:
		return 0;
	}
}

unsigned long long get_timestamp(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return ((unsigned long long)(t.tv_sec)*1000000000LL + t.tv_nsec) / 1000;
}

void sensor_event_to_data(sensor_event_t &event, sensor_data_t &data)
{
	data.data_accuracy = event.data_accuracy;
	data.data_unit_idx = event.data_unit_idx;
	data.time_stamp = event.timestamp;
	data.values_num = event.values_num;
	memcpy(data.values, event.values, sizeof(event.values));
}

void sensorhub_event_to_hub_data(sensorhub_event_t &event, sensorhub_data_t &data)
{
	data.version = event.version;
	data.sensorhub = event.sensorhub;
	data.type = event.type;
	data.hub_data_size = event.hub_data_size;
	data.timestamp = event.timestamp;
	memcpy(data.hub_data, event.hub_data, event.hub_data_size);
	memcpy(data.data, event.data, sizeof(event.data));
}

void print_event_occurrence_log(csensor_handle_info &sensor_handle_info, creg_event_info &event_info,
	sensor_event_data_t &sensor_event_data)
{
	log_attr *log_attr;
	log_map::iterator iter;

	iter = g_log_maps[LOG_ID_EVENT].find(event_info.m_event_type);

	if (iter == g_log_maps[LOG_ID_EVENT].end()) {
		ERR("wrong event_type: 0x%x, handle %d\n", event_info.m_event_type, sensor_handle_info.m_handle);
		return;
	}

	log_attr = iter->second;

	log_attr->cnt++;

	if ((log_attr->cnt != 1) && ((log_attr->cnt % log_attr->print_per_cnt) != 0)) {
		return;
	}

	INFO("%s receives %s with %s[%d][state: %d, option: %d count: %d]", get_client_name(), log_attr->name,
			get_sensor_name(sensor_handle_info.m_sensor_type), sensor_handle_info.m_handle, sensor_handle_info.m_sensor_state,
			sensor_handle_info.m_sensor_option, log_attr->cnt);

	if (event_info.m_event_type == ACCELEROMETER_EVENT_ROTATION_CHECK) {
		INFO("%s\n", get_rotate_name(*(unsigned int*)(sensor_event_data.event_data)));
	}

	INFO("0x%x(cb_event_type = %s, &cb_data, client_data = 0x%x)\n", event_info.m_event_callback,
			log_attr->name, event_info.m_cb_data);
}
