/*
 * sensord
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
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

#include <virtual_sensor.h>
#include <bio_led_green_sensor.h>
#include <sensor_plugin_loader.h>
#include <string.h>
#include <csensor_config.h>

#define SENSOR_NAME "BIO_LED_GREEN_SENSOR"
#define SENSOR_TYPE_BIO_GREEN "BIO_LED_GREEN"

bio_led_green_sensor::bio_led_green_sensor()
: m_bio_sensor(NULL)
, m_green(0.0f)
, m_time(0.0f)
{
	m_name = string(SENSOR_NAME);

	register_supported_event(BIO_LED_GREEN_EVENT_RAW_DATA_REPORT_ON_TIME);
}

bio_led_green_sensor::~bio_led_green_sensor()
{
	INFO("bio_led_green_sensor is destroyed!\n");
}

bool bio_led_green_sensor::init()
{
	sensor_properties_t properties;

	m_bio_sensor = sensor_plugin_loader::get_instance().get_sensor(BIO_SENSOR);

	if (!m_bio_sensor) {
		ERR("Failed to load bio sensor");
		return false;
	}

	m_bio_sensor->get_properties(properties);

	csensor_config &config = csensor_config::get_instance();

	if (!config.is_supported(SENSOR_TYPE_BIO_GREEN, properties.name)) {
		ERR("%s sensor doesn't support bio led green sensor", properties.name.c_str());
		return false;
	}

	set_permission(SENSOR_PERMISSION_BIO);

	INFO("%s is created!\n", sensor_base::get_name());

	return true;
}

sensor_type_t bio_led_green_sensor::get_type(void)
{
	return BIO_LED_GREEN_SENSOR;
}

bool bio_led_green_sensor::on_start(void)
{
	m_bio_sensor->add_client(BIO_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_bio_sensor->start();

	m_time = 0;

	activate();
	return true;
}

bool bio_led_green_sensor::on_stop(void)
{
	m_bio_sensor->delete_client(BIO_EVENT_RAW_DATA_REPORT_ON_TIME);
	m_bio_sensor->stop();

	deactivate();
	return true;
}

bool bio_led_green_sensor::add_interval(int client_id, unsigned int interval, bool is_processor)
{
	m_bio_sensor->add_interval((int)this , interval, true);

	return sensor_base::add_interval(client_id, interval, is_processor);
}

bool bio_led_green_sensor::delete_interval(int client_id, bool is_processor)
{
	m_bio_sensor->delete_interval((int)this , true);

	return sensor_base::delete_interval(client_id, is_processor);
}

void bio_led_green_sensor::synthesize(const sensor_event_t& event, vector<sensor_event_t> &outs)
{

	if (event.event_type == BIO_EVENT_RAW_DATA_REPORT_ON_TIME) {

		sensor_event_t bio_led_green_event;

		float green = event.data.values[5];

		bio_led_green_event.sensor_id = get_id();
		bio_led_green_event.event_type = BIO_LED_GREEN_EVENT_RAW_DATA_REPORT_ON_TIME;
		bio_led_green_event.data.accuracy = SENSOR_ACCURACY_GOOD;
		bio_led_green_event.data.timestamp = event.data.timestamp;
		bio_led_green_event.data.value_count = 1;
		bio_led_green_event.data.values[0] = green;
		outs.push_back(bio_led_green_event);

		AUTOLOCK(m_value_mutex);
		m_green = green;
		m_time = bio_led_green_event.data.timestamp;
		return;
	}
}


int bio_led_green_sensor::get_sensor_data(unsigned int data_id, sensor_data_t &data)
{
	if (data_id != BIO_LED_GREEN_BASE_DATA_SET)
		return -1;

	AUTOLOCK(m_value_mutex);

	data.accuracy = SENSOR_ACCURACY_GOOD;
	data.timestamp = m_time;
	data.values[0] = m_green;
	data.value_count = 1;

	return 0;
}

bool bio_led_green_sensor::get_properties(sensor_properties_t &properties)
{
	m_bio_sensor->get_properties(properties);

	properties.name = "Bio Led Green Sensor";

	return true;
}

extern "C" sensor_module* create(void)
{
	bio_led_green_sensor *sensor;

	try {
		sensor = new(std::nothrow) bio_led_green_sensor;
	} catch (int err) {
		ERR("Failed to create module, err: %d, cause: %s", err, strerror(err));
		return NULL;
	}

	sensor_module *module = new(std::nothrow) sensor_module;
	retvm_if(!module || !sensor, NULL, "Failed to allocate memory");

	module->sensors.push_back(sensor);
	return module;
}
