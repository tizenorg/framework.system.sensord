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
#include <geomagnetic_rv_sensor.h>
#include <sensor_plugin_loader.h>
#include <orientation_filter.h>
#include <cvirtual_sensor_config.h>

using std::string;
using std::vector;

#define SENSOR_NAME "GEOMAGNETIC_RV_SENSOR"
#define SENSOR_TYPE_GEOMAGNETIC_RV		"GEOMAGNETIC_ROTATION_VECTOR"

#define ACCELEROMETER_ENABLED 0x01
#define GEOMAGNETIC_ENABLED 0x02
#define GEOMAGNETIC_RV_ENABLED 3

#define INITIAL_VALUE -1

#define MS_TO_US 1000

#define ELEMENT_NAME											"NAME"
#define ELEMENT_VENDOR											"VENDOR"
#define ELEMENT_RAW_DATA_UNIT									"RAW_DATA_UNIT"
#define ELEMENT_DEFAULT_SAMPLING_TIME							"DEFAULT_SAMPLING_TIME"
#define ELEMENT_ACCEL_STATIC_BIAS								"ACCEL_STATIC_BIAS"
#define ELEMENT_GEOMAGNETIC_STATIC_BIAS							"GEOMAGNETIC_STATIC_BIAS"
#define ELEMENT_ACCEL_ROTATION_DIRECTION_COMPENSATION			"ACCEL_ROTATION_DIRECTION_COMPENSATION"
#define ELEMENT_GEOMAGNETIC_ROTATION_DIRECTION_COMPENSATION		"GEOMAGNETIC_ROTATION_DIRECTION_COMPENSATION"
#define ELEMENT_ACCEL_SCALE										"ACCEL_SCALE"
#define ELEMENT_GEOMAGNETIC_SCALE								"GEOMAGNETIC_SCALE"
#define ELEMENT_MAGNETIC_ALIGNMENT_FACTOR						"MAGNETIC_ALIGNMENT_FACTOR"

void pre_process_data(sensor_data<float> &data_out, const float *data_in, float *bias, int *sign, float scale)
{
	data_out.m_data.m_vec[0] = sign[0] * (data_in[0] - bias[0]) / scale;
	data_out.m_data.m_vec[1] = sign[1] * (data_in[1] - bias[1]) / scale;
	data_out.m_data.m_vec[2] = sign[2] * (data_in[2] - bias[2]) / scale;
}

geomagnetic_rv_sensor::geomagnetic_rv_sensor()
: m_accel_sensor(NULL)
, m_magnetic_sensor(NULL)
, m_accuracy(-1)
, m_time(0)
{
	cvirtual_sensor_config &config = cvirtual_sensor_config::get_instance();

	m_name = string(SENSOR_NAME);
	register_supported_event(GEOMAGNETIC_RV_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_enable_geomagnetic_rv = 0;

	if (!config.get(SENSOR_TYPE_GEOMAGNETIC_RV, ELEMENT_VENDOR, m_vendor)) {
		ERR("[VENDOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_vendor = %s", m_vendor.c_str());

	if (!config.get(SENSOR_TYPE_GEOMAGNETIC_RV, ELEMENT_DEFAULT_SAMPLING_TIME, &m_default_sampling_time)) {
		ERR("[DEFAULT_SAMPLING_TIME] is empty\n");
		throw ENXIO;
	}

	INFO("m_default_sampling_time = %d", m_default_sampling_time);

	if (!config.get(SENSOR_TYPE_GEOMAGNETIC_RV, ELEMENT_ACCEL_STATIC_BIAS, m_accel_static_bias, 3)) {
		ERR("[ACCEL_STATIC_BIAS] is empty\n");
		throw ENXIO;
	}

	INFO("m_accel_static_bias = (%f, %f, %f)", m_accel_static_bias[0], m_accel_static_bias[1], m_accel_static_bias[2]);

	if (!config.get(SENSOR_TYPE_GEOMAGNETIC_RV, ELEMENT_GEOMAGNETIC_STATIC_BIAS, m_geomagnetic_static_bias, 3)) {
		ERR("[GEOMAGNETIC_STATIC_BIAS] is empty\n");
		throw ENXIO;
	}

	INFO("m_geomagnetic_static_bias = (%f, %f, %f)", m_geomagnetic_static_bias[0], m_geomagnetic_static_bias[1], m_geomagnetic_static_bias[2]);

	if (!config.get(SENSOR_TYPE_GEOMAGNETIC_RV, ELEMENT_ACCEL_ROTATION_DIRECTION_COMPENSATION, m_accel_rotation_direction_compensation, 3)) {
		ERR("[ACCEL_ROTATION_DIRECTION_COMPENSATION] is empty\n");
		throw ENXIO;
	}

	INFO("m_accel_rotation_direction_compensation = (%d, %d, %d)", m_accel_rotation_direction_compensation[0], m_accel_rotation_direction_compensation[1], m_accel_rotation_direction_compensation[2]);

	if (!config.get(SENSOR_TYPE_GEOMAGNETIC_RV, ELEMENT_GEOMAGNETIC_ROTATION_DIRECTION_COMPENSATION, m_geomagnetic_rotation_direction_compensation, 3)) {
		ERR("[GEOMAGNETIC_ROTATION_DIRECTION_COMPENSATION] is empty\n");
		throw ENXIO;
	}

	INFO("m_geomagnetic_rotation_direction_compensation = (%d, %d, %d)", m_geomagnetic_rotation_direction_compensation[0], m_geomagnetic_rotation_direction_compensation[1], m_geomagnetic_rotation_direction_compensation[2]);

	if (!config.get(SENSOR_TYPE_GEOMAGNETIC_RV, ELEMENT_ACCEL_SCALE, &m_accel_scale)) {
		ERR("[ACCEL_SCALE] is empty\n");
		throw ENXIO;
	}

	INFO("m_accel_scale = %f", m_accel_scale);

	if (!config.get(SENSOR_TYPE_GEOMAGNETIC_RV, ELEMENT_GEOMAGNETIC_SCALE, &m_geomagnetic_scale)) {
		ERR("[GEOMAGNETIC_SCALE] is empty\n");
		throw ENXIO;
	}

	INFO("m_geomagnetic_scale = %f", m_geomagnetic_scale);

	if (!config.get(SENSOR_TYPE_GEOMAGNETIC_RV, ELEMENT_MAGNETIC_ALIGNMENT_FACTOR, &m_magnetic_alignment_factor)) {
		ERR("[MAGNETIC_ALIGNMENT_FACTOR] is empty\n");
		throw ENXIO;
	}

	INFO("m_magnetic_alignment_factor = %d", m_magnetic_alignment_factor);

	m_interval = m_default_sampling_time * MS_TO_US;

	m_accel_ptr = m_magnetic_ptr = NULL;
}

geomagnetic_rv_sensor::~geomagnetic_rv_sensor()
{
	INFO("geomagnetic_rv_sensor is destroyed!\n");
}

bool geomagnetic_rv_sensor::init()
{
	m_accel_sensor = sensor_plugin_loader::get_instance().get_sensor(ACCELEROMETER_SENSOR);
	m_magnetic_sensor = sensor_plugin_loader::get_instance().get_sensor(GEOMAGNETIC_SENSOR);

	if (!m_accel_sensor || !m_magnetic_sensor) {
		ERR("Failed to load sensors,  accel: 0x%x, mag: 0x%x",
			m_accel_sensor, m_magnetic_sensor);
		return false;
	}

	INFO("%s is created!\n", sensor_base::get_name());

	return true;
}

void geomagnetic_rv_sensor::get_types(vector<sensor_type_t> &types)
{
	types.push_back(GEOMAGNETIC_RV_SENSOR);
}

bool geomagnetic_rv_sensor::on_start(void)
{
	AUTOLOCK(m_mutex);

	m_accel_sensor->add_client(ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_accel_sensor->add_interval((intptr_t)this, (m_interval/MS_TO_US), false);
	m_accel_sensor->start();
	m_magnetic_sensor->add_client(GEOMAGNETIC_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_magnetic_sensor->add_interval((intptr_t)this, (m_interval/MS_TO_US), false);
	m_magnetic_sensor->start();

	activate();
	return true;
}

bool geomagnetic_rv_sensor::on_stop(void)
{
	AUTOLOCK(m_mutex);

	m_accel_sensor->delete_client(ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_accel_sensor->delete_interval((intptr_t)this, false);
	m_accel_sensor->stop();
	m_magnetic_sensor->delete_client(GEOMAGNETIC_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_magnetic_sensor->delete_interval((intptr_t)this, false);
	m_magnetic_sensor->stop();

	deactivate();
	return true;
}

bool geomagnetic_rv_sensor::add_interval(int client_id, unsigned int interval)
{
	AUTOLOCK(m_mutex);

	m_accel_sensor->add_interval(client_id, interval, false);
	m_magnetic_sensor->add_interval(client_id, interval, false);

	return sensor_base::add_interval(client_id, interval, false);
}

bool geomagnetic_rv_sensor::delete_interval(int client_id)
{
	AUTOLOCK(m_mutex);

	m_accel_sensor->delete_interval(client_id, false);
	m_magnetic_sensor->delete_interval(client_id, false);

	return sensor_base::delete_interval(client_id, false);
}

void geomagnetic_rv_sensor::synthesize(const sensor_event_t& event, vector<sensor_event_t> &outs)
{
	const float MIN_DELIVERY_DIFF_FACTOR = 0.75f;
	unsigned long long diff_time;

	sensor_event_t rv_event;
	quaternion<float> quaternion_geo_rv;

	if (event.event_type == ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME) {
		diff_time = event.data.timestamp - m_time;

		if (m_time && (diff_time < m_interval * MIN_DELIVERY_DIFF_FACTOR))
			return;

		pre_process_data(m_accel, event.data.values, m_accel_static_bias, m_accel_rotation_direction_compensation, m_accel_scale);

		m_accel.m_time_stamp = event.data.timestamp;

		m_accel_ptr = &m_accel;

		m_enable_geomagnetic_rv |= ACCELEROMETER_ENABLED;
	}
	else if (event.event_type == GEOMAGNETIC_EVENT_RAW_DATA_REPORT_ON_TIME) {
		diff_time = event.data.timestamp - m_time;

		if (m_time && (diff_time < m_interval * MIN_DELIVERY_DIFF_FACTOR))
			return;

		pre_process_data(m_magnetic, event.data.values, m_geomagnetic_static_bias, m_geomagnetic_rotation_direction_compensation, m_geomagnetic_scale);

		m_magnetic.m_time_stamp = event.data.timestamp;

		m_magnetic_ptr = &m_magnetic;

		m_enable_geomagnetic_rv |= GEOMAGNETIC_ENABLED;
	}

	if (m_enable_geomagnetic_rv == GEOMAGNETIC_RV_ENABLED) {
		m_enable_geomagnetic_rv = 0;

		m_orientation_filter.m_magnetic_alignment_factor = m_magnetic_alignment_factor;

		{
			AUTOLOCK(m_fusion_mutex);
			quaternion_geo_rv = m_orientation_filter.get_quaternion(m_accel_ptr, NULL, m_magnetic_ptr);
		}

		m_time = get_timestamp();
		rv_event.sensor_id = get_id();
		rv_event.event_type = GEOMAGNETIC_RV_EVENT_RAW_DATA_REPORT_ON_TIME;
		rv_event.data.accuracy = SENSOR_ACCURACY_GOOD;
		rv_event.data.timestamp = m_time;
		rv_event.data.value_count = 4;
		rv_event.data.values[0] = quaternion_geo_rv.m_quat.m_vec[1];
		rv_event.data.values[1] = quaternion_geo_rv.m_quat.m_vec[2];
		rv_event.data.values[2] = quaternion_geo_rv.m_quat.m_vec[3];
		rv_event.data.values[3] = quaternion_geo_rv.m_quat.m_vec[0];

		push(rv_event);

		m_accel_ptr = m_magnetic_ptr = NULL;

		{
			AUTOLOCK(m_value_mutex);
			m_time = rv_event.data.value_count;
			m_x = rv_event.data.values[0];
			m_y = rv_event.data.values[1];
			m_z = rv_event.data.values[2];
			m_w = rv_event.data.values[3];
		}
	}

	return;
}

int geomagnetic_rv_sensor::get_sensor_data(unsigned int data_id, sensor_data_t &data)
{
	if (data_id != GEOMAGNETIC_RV_EVENT_RAW_DATA_REPORT_ON_TIME)
		return -1;

	data.accuracy = SENSOR_ACCURACY_GOOD;

	AUTOLOCK(m_value_mutex);
	data.timestamp = m_time;
	data.values[0] = m_x;
	data.values[1] = m_y;
	data.values[2] = m_z;
	data.values[3] = m_w;
	data.value_count = 4;

	return 0;
}

bool geomagnetic_rv_sensor::get_properties(sensor_type_t sensor_type, sensor_properties_t &properties)
{
	properties.vendor = m_vendor;
	properties.name = SENSOR_NAME;
	properties.min_range = -1;
	properties.max_range = 1;
	properties.resolution = 0.000001;
	properties.fifo_count = 0;
	properties.max_batch_count = 0;
	properties.min_interval = 1;

	return true;
}

extern "C" sensor_module* create(void)
{
	geomagnetic_rv_sensor *sensor;

	try {
		sensor = new(std::nothrow) geomagnetic_rv_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
