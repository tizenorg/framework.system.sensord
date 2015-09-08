/*
 * bio_hrm_sensor_hal
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

#ifndef _BIO_HRM_SENSOR_HAL_H_
#define _BIO_HRM_SENSOR_HAL_H_

#include <sensor_hal.h>
#include <string>

using std::string;

class bio_hrm_sensor_hal : public sensor_hal
{
public:
	bio_hrm_sensor_hal();
	virtual ~bio_hrm_sensor_hal();
	sensor_type_t get_type(void);
	string get_model_id(void);
	bool enable(void);
	bool disable(void);
	bool set_interval(unsigned long val);
	bool is_data_ready(bool wait);
	virtual int get_sensor_data(sensor_data_t &data);
	bool get_properties(sensor_properties_t &properties);
private:
	string m_model_id;
	string m_name;
	string m_vendor;
	string m_chip_name;

	string m_enable_node;
	string m_data_node;
	string m_interval_node;


	int m_hr;
	int m_spo2;
	int m_peek_to_peek;
	float m_snr;

	unsigned long long m_fired_time;
	float m_raw_data_unit;
	int m_node_handle;
	unsigned long m_polling_interval;
	bool m_sensorhub_controlled;

	cmutex m_value_mutex;

	bool update_value(bool wait);
};
#endif /*_GYRO_SENSOR_HAL_CLASS_H_*/
