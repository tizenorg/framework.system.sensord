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

#ifndef _AUTO_ROTATION_SENSOR_H_
#define _AUTO_ROTATION_SENSOR_H_

#include <sensor_internal.h>
#include <auto_rotation_alg.h>

class auto_rotation_sensor : public virtual_sensor {
public:
	auto_rotation_sensor();
	virtual ~auto_rotation_sensor();

	bool init();
	virtual void get_types(std::vector<sensor_type_t> &types);

	static bool working(void *inst);

	void synthesize(const sensor_event_t& event, std::vector<sensor_event_t> &outs);

	int get_sensor_data(const unsigned int data_id, sensor_data_t &data);
	virtual bool get_properties(sensor_type_t sensor_type, sensor_properties_t &properties);
private:
	sensor_base *m_accel_sensor;
	cmutex m_value_mutex;

	int m_rotation;
	unsigned int m_interval;
	unsigned long long m_rotation_time;
	auto_rotation_alg *m_alg;

	static const int SAMPLING_TIME = 200; // fix for slow auto-rotation

	std::string m_vendor;
	std::string m_raw_data_unit;
	int m_default_sampling_time;

	auto_rotation_alg *get_alg();
	virtual bool on_start(void);
	virtual bool on_stop(void);
	bool check_lib(void);
};

#endif
