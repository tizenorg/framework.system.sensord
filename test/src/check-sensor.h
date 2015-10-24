/*
 * sensord
 *
 * Copyright (c) 2014-15 Samsung Electronics Co., Ltd.
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
#ifndef CHECK_SENSOR_H
#define CHECK_SENSOR_H

#define DEFAULT_EVENT_INTERVAL 100

int get_event(sensor_type_t sensor_type, char str[]);
void callback(sensor_t sensor, unsigned int event_type, sensor_data_t *data, void *user_data);
int check_sensor(sensor_type_t sensor_type, unsigned int event, int interval);
void printpollinglogs(sensor_type_t type, sensor_data_t data);
int polling_sensor(sensor_type_t sensor_type, unsigned int event);

#endif
