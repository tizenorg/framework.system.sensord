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

#ifndef __SENSOR_INTERNAL_DEPRECATED__
#define __SENSOR_INTERNAL_DEPRECATED__

#pragma GCC diagnostic warning "-Wunused-value"
#pragma GCC diagnostic warning "-Wunused-variable"
#pragma GCC diagnostic warning "-Wunused-but-set-variable"
#pragma GCC diagnostic warning "-Wmaybe-uninitialized"

#ifndef DEPRECATED
#define DEPRECATED __attribute__((deprecated))
#endif

#include "stdbool.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>

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
#include <sensor_rv_raw.h>
#include <sensor_geomagnetic_rv.h>
#include <sensor_gaming_rv.h>
#include <sensor_pir.h>
#include <sensor_pir_long.h>
#include <sensor_temperature.h>
#include <sensor_humidity.h>
#include <sensor_ultraviolet.h>
#include <sensor_bio_led_green.h>

typedef struct {
	condition_op_t cond_op;
	float cond_value1;
} event_condition_t;

typedef struct {
	size_t event_data_size;
	void *event_data;
} sensor_event_data_t;

typedef void (*sensor_callback_func_t)(unsigned int, sensor_event_data_t *, void *);
typedef sensor_callback_func_t sensor_legacy_cb_t;

typedef struct {
	int x;
	int y;
	int z;
} sensor_panning_data_t;

#define sf_connect(sensor_type) -1
#define sf_disconnect(handle) -1
#define sf_start(handle, option) -1
#define sf_stop(handle) -1
#define sf_register_event(handle, event_type, event_condition, cb, user_data) -1
#define sf_unregister_event(handle, event_type) -1
#define sf_get_data(handle, data_id, values) -1
#define sf_change_event_condition(handle, event_type, event_condition) -1
#define sf_change_sensor_option(handle, option) -1
#define sf_send_sensorhub_data(handle, data, data_len) -1


#ifdef __cplusplus
}
#endif


#endif
