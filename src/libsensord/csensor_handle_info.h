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

#ifndef CSENSOR_HANDLE_INFO_H_
#define CSENSOR_HANDLE_INFO_H_

#include <creg_event_info.h>
#include <command_channel.h>
#include <common.h>
#include <string.h>
#include <map>
#include <vector>
using std::map;
using std::vector;

typedef map<unsigned int,creg_event_info> event_info_map;

class csensor_handle_info {
public:
	int m_handle;
	sensor_type m_sensor_type;
	int m_sensor_state;
	int m_sensor_option;
	int bad_accuracy;

	csensor_handle_info();
	~csensor_handle_info();

	bool add_reg_event_info(const unsigned int event_type, const unsigned int interval,
			const sensor_callback_func_t callback,void* cb_data);
	bool delete_reg_event_info(const unsigned int event_type);

	bool change_reg_event_interval(const unsigned int event_type, const unsigned int interval);

	bool get_reg_event_info(const unsigned int event_type, creg_event_info &event_info);
	void get_reg_event_types(event_type_vector &event_types);
	unsigned int get_min_interval(void);
	unsigned int get_reg_event_count(void);

	void clear_all_events(void);
private:
	event_info_map m_reg_event_infos;
};


#endif /* CSENSOR_HANDLE_INFO_H_ */
