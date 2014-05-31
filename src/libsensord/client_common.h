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

#ifndef CLIENT_COMMON_H_
#define CLIENT_COMMON_H_

/*header for each sensor type*/
#include <sensor.h>
#include <csensor_handle_info.h>
#include <creg_event_info.h>
#include <common.h>

#define BASE_GATHERING_INTERVAL	100

#define CLIENT_NAME_SIZE NAME_MAX+10

#define ROTATION_0 0
#define ROTATION_90 90
#define ROTATION_180 180
#define ROTATION_270 270
#define ROTATION_360 360
#define ROTATION_THD 45

enum log_id {
	LOG_ID_START = 0,
	LOG_ID_SENSOR_TYPE = 0,
	LOG_ID_EVENT,
	LOG_ID_DATA,
	LOG_ID_PROPERTY,
	LOG_ID_ROTATE,
	LOG_ID_END,
};

struct log_attr {
	const char *name;
	unsigned long cnt;
	const unsigned int print_per_cnt;
};

struct log_element {
	log_id id;
	unsigned int type;
	struct log_attr log_attr;
};


typedef struct {
	int handle;
	unsigned int event_type;
	sensor_event_data_t ev_data;
	int sensor_state;
	int sensor_option;
	sensor_type sensor;
	creg_event_info event_info;
} log_info;

bool is_sensorhub_event(unsigned int event_type);
bool is_one_shot_event(unsigned int event_type);
bool is_ontime_event(unsigned int event_type);
bool is_panning_event(unsigned int event_type);
bool is_single_state_event(unsigned int event_type);
unsigned int get_calibration_event_type(unsigned int event_type);
unsigned long long get_timestamp(void);
void sensor_event_to_data(sensor_event_t &event, sensor_data_t &data);
void sensorhub_event_to_hub_data(sensorhub_event_t &event, sensorhub_data_t &data);
bool is_sensor_supported(sensor_type sensor);

const char* get_log_element_name(log_id id, unsigned int type);
const char* get_sensor_name(sensor_type sensor);
const char* get_event_name(unsigned int event_type);
const char* get_data_name(unsigned int data_id);
const char* get_rotate_name(unsigned int rotate_type);
void print_event_occurrence_log(csensor_handle_info &sensor_handle_info, creg_event_info &event_info,
	sensor_event_data_t &event_data);

#endif /* CLIENT_COMMON_H_ */
