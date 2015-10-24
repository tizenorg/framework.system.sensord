/*
 * libsensord
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

#ifndef __SENSOR_INTERNAL_H__
#define __SENSOR_INTERNAL_H__

#ifndef DEPRECATED
#define DEPRECATED __attribute__((deprecated))
#endif

#include "stdbool.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>

#include <sensor_internal_deprecated.h>

/*header for common sensor type*/
#include <sensor_common.h>

/*header for each sensor type*/
#include <sensor_accel.h>
#include <sensor_geomag.h>
#include <sensor_geomag_uncal.h>
#include <sensor_light.h>
#include <sensor_proxi.h>
#include <sensor_motion.h>
#include <sensor_gyro.h>
#include <sensor_gyro_uncal.h>
#include <sensor_pressure.h>
#include <sensor_pedo.h>
#include <sensor_context.h>
#include <sensor_flat.h>
#include <sensor_bio.h>
#include <sensor_bio_hrm.h>
#include <sensor_auto_rotation.h>
#include <sensor_gravity.h>
#include <sensor_linear_accel.h>
#include <sensor_orientation.h>
#include <sensor_rv.h>
#include <sensor_geomagnetic_rv.h>
#include <sensor_gaming_rv.h>
#include <sensor_rv_raw.h>
#include <sensor_pir.h>
#include <sensor_pir_long.h>
#include <sensor_temperature.h>
#include <sensor_humidity.h>
#include <sensor_ultraviolet.h>
#include <sensor_dust.h>
#include <sensor_bio_led_green.h>

typedef void (*sensor_cb_t)(sensor_t sensor, unsigned int event_type, sensor_data_t *data, void *user_data);
typedef void (*sensorhub_cb_t)(sensor_t sensor, unsigned int event_type, sensorhub_data_t *data, void *user_data);
typedef void (*sensor_accuracy_changed_cb_t) (sensor_t sensor, unsigned long long timestamp, int accuracy, void *user_data);

#define sensord_get_sensor_list(type, list, sensor_count) false
#define sensord_get_sensor(type) NULL
#define sensord_get_type(sensor, type) false
#define sensord_get_name(sensor) NULL
#define sensord_get_vendor(sensor) NULL
#define sensord_get_privilege(sensor, privilege) false
#define sensord_get_min_range(sensor, min_range) false
#define sensord_get_max_range(sensor, max_range) false
#define sensord_get_resolution(sensor, resolution) false
#define sensord_get_min_interval(sensor, min_interval) false
#define sensord_get_fifo_count(sensor, fifo_count) false
#define sensord_get_max_batch_count(sensor, max_batch_count) false
#define sensord_get_supported_event_types(sensor, event_types, count) false
#define sensord_is_supported_event_type(sensor, event_type, supported) false
#define sensord_connect(sensor) -1
#define sensord_disconnect(handle) false
#define sensord_register_event(handle, event_type, interval, max_batch_latency, cb, user_data) false
#define sensord_register_hub_event(handle, event_type, interval, max_batch_latency, cb, user_data) false
#define sensord_unregister_event(handle, event_type) false
#define sensord_register_accuracy_cb(handle, cb, user_data) false
#define sensord_unregister_accuracy_cb(handle) false
#define sensord_start(handle, option) false
#define sensord_stop(handle) false
#define sensord_change_event_interval(handle, event_type, interval) false
#define sensord_change_event_max_batch_latency(handle, max_batch_latency) false
#define sensord_set_option(handle, option) false
#define sensord_send_sensorhub_data(handle, data, data_len) false
#define sensord_get_data(handle, data_id, sensor_data) false

#ifdef __cplusplus
}
#endif


#endif
