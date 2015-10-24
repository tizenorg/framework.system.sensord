/*
 * sensord
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
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
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <common.h>
#include <sf_common.h>
#include <tilt_sensor.h>
#include <sensor_plugin_loader.h>
#include <orientation_filter.h>
#include <cvirtual_sensor_config.h>

using std::string;
using std::vector;

#define SENSOR_NAME			"TILT_SENSOR"
#define SENSOR_TYPE_TILT	"TILT"

#define MIN_DELIVERY_DIFF_FACTOR 0.75f

#define INITIAL_VALUE -1

#define MS_TO_US 1000

#define ACCELEROMETER_ENABLED 0x01
#define TILT_ENABLED 1

#define ELEMENT_NAME											"NAME"
#define ELEMENT_VENDOR											"VENDOR"
#define ELEMENT_RAW_DATA_UNIT									"RAW_DATA_UNIT"
#define ELEMENT_DEFAULT_SAMPLING_TIME							"DEFAULT_SAMPLING_TIME"
#define ELEMENT_ACCEL_STATIC_BIAS								"ACCEL_STATIC_BIAS"
#define ELEMENT_ACCEL_ROTATION_DIRECTION_COMPENSATION			"ACCEL_ROTATION_DIRECTION_COMPENSATION"
#define ELEMENT_ACCEL_SCALE										"ACCEL_SCALE"
#define ELEMENT_PITCH_ROTATION_COMPENSATION						"PITCH_ROTATION_COMPENSATION"
#define ELEMENT_ROLL_ROTATION_COMPENSATION						"ROLL_ROTATION_COMPENSATION"

tilt_sensor::tilt_sensor()
: m_accel_sensor(NULL)
, m_time(0)
{
	cvirtual_sensor_config &config = cvirtual_sensor_config::get_instance();

	m_name = string(SENSOR_NAME);
	register_supported_event(TILT_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_enable_tilt = 0;

	if (!config.get(SENSOR_TYPE_TILT, ELEMENT_VENDOR, m_vendor)) {
		ERR("[VENDOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_vendor = %s", m_vendor.c_str());

	if (!config.get(SENSOR_TYPE_TILT, ELEMENT_RAW_DATA_UNIT, m_raw_data_unit)) {
		ERR("[RAW_DATA_UNIT] is empty\n");
		throw ENXIO;
	}

	INFO("m_raw_data_unit = %s", m_raw_data_unit.c_str());

	if (!config.get(SENSOR_TYPE_TILT, ELEMENT_DEFAULT_SAMPLING_TIME, &m_default_sampling_time)) {
		ERR("[DEFAULT_SAMPLING_TIME] is empty\n");
		throw ENXIO;
	}

	INFO("m_default_sampling_time = %d", m_default_sampling_time);

	if (!config.get(SENSOR_TYPE_TILT, ELEMENT_PITCH_ROTATION_COMPENSATION, &m_pitch_rotation_compensation)) {
		ERR("[PITCH_ROTATION_COMPENSATION] is empty\n");
		throw ENXIO;
	}

	INFO("m_pitch_rotation_compensation = %d", m_pitch_rotation_compensation);

	if (!config.get(SENSOR_TYPE_TILT, ELEMENT_ROLL_ROTATION_COMPENSATION, &m_roll_rotation_compensation)) {
		ERR("[ROLL_ROTATION_COMPENSATION] is empty\n");
		throw ENXIO;
	}

	INFO("m_roll_rotation_compensation = %d", m_roll_rotation_compensation);

	if (!config.get(SENSOR_TYPE_TILT, ELEMENT_ACCEL_STATIC_BIAS, m_accel_static_bias, 3)) {
		ERR("[ACCEL_STATIC_BIAS] is empty\n");
		throw ENXIO;
	}

	if (!config.get(SENSOR_TYPE_TILT, ELEMENT_ACCEL_ROTATION_DIRECTION_COMPENSATION, m_accel_rotation_direction_compensation, 3)) {
		ERR("[ACCEL_ROTATION_DIRECTION_COMPENSATION] is empty\n");
		throw ENXIO;
	}

	INFO("m_accel_rotation_direction_compensation = (%d, %d, %d)", m_accel_rotation_direction_compensation[0], m_accel_rotation_direction_compensation[1], m_accel_rotation_direction_compensation[2]);

	if (!config.get(SENSOR_TYPE_TILT, ELEMENT_ACCEL_SCALE, &m_accel_scale)) {
		ERR("[ACCEL_SCALE] is empty\n");
		throw ENXIO;
	}

	INFO("m_accel_scale = %f", m_accel_scale);

	m_interval = m_default_sampling_time * MS_TO_US;
}

tilt_sensor::~tilt_sensor()
{
	INFO("tilt_sensor is destroyed!\n");
}

bool tilt_sensor::init(void)
{
	m_accel_sensor = sensor_plugin_loader::get_instance().get_sensor(ACCELEROMETER_SENSOR);

	if (!m_accel_sensor) {
		ERR("Failed to load sensors,  accel: 0x%x", m_accel_sensor);
		return false;
	}

	INFO("%s is created!\n", sensor_base::get_name());

	return true;
}

void tilt_sensor::get_types(vector<sensor_type_t> &types)
{
	types.push_back(TILT_SENSOR);
}

bool tilt_sensor::on_start(void)
{
	AUTOLOCK(m_mutex);

	m_accel_sensor->add_client(ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_accel_sensor->add_interval((intptr_t)this, (m_interval/MS_TO_US), false);
	m_accel_sensor->start();

	activate();
	return true;
}

bool tilt_sensor::on_stop(void)
{
	AUTOLOCK(m_mutex);

	m_accel_sensor->delete_client(ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_accel_sensor->delete_interval((intptr_t)this, false);
	m_accel_sensor->stop();

	deactivate();
	return true;
}

bool tilt_sensor::add_interval(int client_id, unsigned int interval)
{
	AUTOLOCK(m_mutex);

	m_accel_sensor->add_interval(client_id, interval, false);

	return sensor_base::add_interval(client_id, interval, false);
}

bool tilt_sensor::delete_interval(int client_id)
{
	AUTOLOCK(m_mutex);

	m_accel_sensor->delete_interval(client_id, false);

	return sensor_base::delete_interval(client_id, false);
}

void tilt_sensor::synthesize(const sensor_event_t &event, vector<sensor_event_t> &outs)
{
	unsigned long long diff_time;

	if (event.event_type == ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME) {

		diff_time = event.data.timestamp - m_time;

		if (m_time && (diff_time < m_interval * MIN_DELIVERY_DIFF_FACTOR))
			return;
		m_accel.m_data.m_vec[0] = m_accel_rotation_direction_compensation[0] * (event.data.values[0] - m_accel_static_bias[0]) / m_accel_scale;
		m_accel.m_data.m_vec[1] = m_accel_rotation_direction_compensation[1] * (event.data.values[1] - m_accel_static_bias[1]) / m_accel_scale;
		m_accel.m_data.m_vec[2] = m_accel_rotation_direction_compensation[2] * (event.data.values[2] - m_accel_static_bias[2]) / m_accel_scale;

		m_accel.m_time_stamp = event.data.timestamp;

		m_enable_tilt |= ACCELEROMETER_ENABLED;
	}

	if (m_enable_tilt == TILT_ENABLED) {
		m_enable_tilt = 0;

		sensor_event_t tilt_event;

		quaternion<float> quat = m_orientation_filter.get_quaternion(&m_accel, NULL, NULL);

		euler_angles<float> euler = quat2euler(quat);

		if(m_raw_data_unit == "DEGREES") {
			euler = rad2deg(euler);
		}

		euler.m_ang.m_vec[0] *= m_pitch_rotation_compensation;
		euler.m_ang.m_vec[1] *= m_roll_rotation_compensation;

		m_time = get_timestamp();
		tilt_event.sensor_id = get_id();
		tilt_event.event_type = TILT_EVENT_RAW_DATA_REPORT_ON_TIME;
		tilt_event.data.accuracy = event.data.accuracy;
		tilt_event.data.timestamp = m_time;
		tilt_event.data.value_count = 2;
		tilt_event.data.values[0] = euler.m_ang.m_vec[0];
		tilt_event.data.values[1] = euler.m_ang.m_vec[1];

		push(tilt_event);

		{
			AUTOLOCK(m_value_mutex);
			m_time = tilt_event.data.timestamp;
			m_x = tilt_event.data.values[0];
			m_y = tilt_event.data.values[1];
		}
	}

	return;
}

int tilt_sensor::get_sensor_data(const unsigned int event_type, sensor_data_t &data)
{
	if (event_type != TILT_BASE_DATA_SET)
		return -1;

	AUTOLOCK(m_value_mutex);
	data.accuracy = SENSOR_ACCURACY_GOOD;
	data.timestamp = m_time;
	data.values[0] = m_x;
	data.values[1] = m_y;
	data.value_count = 2;
	return 0;
}

bool tilt_sensor::get_properties(sensor_type_t sensor_type, sensor_properties_t &properties)
{
	if(m_raw_data_unit == "DEGREES") {
		properties.min_range = -180;
		properties.max_range = 180;
	}
	else {
		properties.min_range = -PI;
		properties.max_range = PI;
	}
	properties.resolution = 0.000001;
	properties.vendor = m_vendor;
	properties.name = SENSOR_NAME;
	properties.min_interval = 1;
	properties.fifo_count = 0;
	properties.max_batch_count = 0;

	return true;
}

extern "C" sensor_module* create(void)
{
	tilt_sensor *sensor;

	try {
		sensor = new(std::nothrow) tilt_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
