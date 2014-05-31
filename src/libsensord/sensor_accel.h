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

#ifndef __SENSOR_ACCEL_H__
#define __SENSOR_ACCEL_H__

//! Pre-defined events for the accelometer sensor
//! Sensor Plugin developer can add more event to their own headers

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @defgroup SENSOR_ACCEL Accelerometer Sensor
 * @ingroup SENSOR_FRAMEWORK
 * 
 * These APIs are used to control the Accelerometer sensor.
 * @{
 */

enum accelerometer_data_id {
	ACCELEROMETER_BASE_DATA_SET 				= (ACCELEROMETER_SENSOR << 16) | 0x0001,
	ACCELEROMETER_ORIENTATION_DATA_SET 			= (ACCELEROMETER_SENSOR << 16) | 0x0002,
	ACCELEROMETER_LINEAR_ACCELERATION_DATA_SET 	= (ACCELEROMETER_SENSOR << 16) | 0x0004,
	ACCELEROMETER_GRAVITY_DATA_SET      		= (ACCELEROMETER_SENSOR << 16) | 0x0008,
	ACCELEROMETER_ROTATION_DATA_SET      		= (ACCELEROMETER_SENSOR << 16) | 0x0010,
};

enum accelerometer_event_type {		
	ACCELEROMETER_EVENT_ROTATION_CHECK		    				= (ACCELEROMETER_SENSOR << 16) | 0x0001,
	ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME					= (ACCELEROMETER_SENSOR << 16) | 0x0002,
	ACCELEROMETER_EVENT_CALIBRATION_NEEDED						= (ACCELEROMETER_SENSOR << 16) | 0x0004,
	ACCELEROMETER_EVENT_SET_HORIZON								= (ACCELEROMETER_SENSOR << 16) | 0x0008,
	ACCELEROMETER_EVENT_SET_WAKEUP 								= (ACCELEROMETER_SENSOR << 16) | 0x0010,
	ACCELEROMETER_EVENT_ORIENTATION_DATA_REPORT_ON_TIME 		= (ACCELEROMETER_SENSOR << 16) | 0x0020,
	ACCELEROMETER_EVENT_LINEAR_ACCELERATION_DATA_REPORT_ON_TIME = (ACCELEROMETER_SENSOR << 16) | 0x0040,
	ACCELEROMETER_EVENT_GRAVITY_DATA_REPORT_ON_TIME 			= (ACCELEROMETER_SENSOR << 16) | 0x0080,
};

enum accelerometer_rotate_state {
	ROTATION_UNKNOWN	 		= 0,
	ROTATION_LANDSCAPE_LEFT		= 1,
	ROTATION_PORTRAIT_TOP		= 2,
	ROTATION_PORTRAIT_BTM		= 3,
	ROTATION_LANDSCAPE_RIGHT	= 4,
	ROTATION_EVENT_0			= 2,	/*CCW base*/
	ROTATION_EVENT_90			= 1,	/*CCW base*/
	ROTATION_EVENT_180			= 3,	/*CCW base*/
	ROTATION_EVENT_270			= 4,	/*CCW base*/
};

enum accelerometer_property_id {
	ACCELEROMETER_PROPERTY_UNKNOWN = 0,
	ACCELEROMETER_PROPERTY_SET_CALIBRATION,
	ACCELEROMETER_PROPERTY_CHECK_CALIBRATION_STATUS,
	ACCELEROMETER_PROPERTY_SET_WAKEUP,
	ACCELEROMETER_PROPERTY_CHECK_WAKEUP_STATUS,
	ACCELEROMETER_PROPERTY_CHECK_WAKEUP_SUPPORTED,
	ACCELEROMETER_PROPERTY_GET_WAKEUP,
};

enum accelerometer_wakeup_state {
	WAKEUP_UNSET	= 0,
	WAKEUP_SET		= 1,
};

struct rotation_event {
	enum accelerometer_rotate_state rotation;
	enum accelerometer_rotate_state rm[2];
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
//! End of a file
