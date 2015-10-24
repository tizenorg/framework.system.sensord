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

#ifndef _PIR_SENSOR_H_
#define _PIR_SENSOR_H_

#include <sensor_common.h>

#include <physical_sensor.h>
#include <sensor_hal.h>

class pir_sensor : public physical_sensor {
public:
	pir_sensor();
	virtual ~pir_sensor();

	bool init();
	virtual void get_types(std::vector<sensor_type_t> &types);

	static bool working(void *inst);

	virtual bool set_interval(unsigned long interval);
	int get_sensor_data(unsigned int type, sensor_data_t &data);
	virtual bool get_properties(sensor_type_t sensor_type, sensor_properties_t &properties);
private:
	sensor_hal *m_sensor_hal;

	int m_pir;

	cmutex m_value_mutex;

	virtual bool on_start(void);
	virtual bool on_stop(void);

	void raw_to_state(sensor_data_t &data);
	bool process_event(void);
};

#endif

