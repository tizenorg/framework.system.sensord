/*
 * libsf-common
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

#if !defined(_SF_COMMON_H_)
#define _SF_COMMON_H_
#include <cpacket.h>
#include <csock.h>
#include <cobject_type.h>
#include <vector>

#define DEFAULT_SENSOR_KEY_PREFIX		"memory/private/sensor/"
#define MAX_KEY_LEN		30

#define COMMAND_CHANNEL_PATH			"/tmp/sf_command_socket"
#define EVENT_CHANNEL_PATH				"/tmp/sf_event_socket"

#define MAX_DATA_STREAM_SIZE	(sizeof(cmd_get_struct_t) + sizeof(base_data_struct) + 8)	/*always check this size when a new cmd_type struct added*/
#define MAX_VALUE_SIZE 12

#define MAX_HANDLE			64
#define MAX_HANDLE_REACHED	-2

#define CLIENT_ID_INVALID   -1

static const int SF_PLUGIN_BASE = ctype::UNKNOWN + 0x10000000;

enum event_situation {
	SITUATION_LCD_ON,
	SITUATION_LCD_OFF,
	SITUATION_SURVIVAL_MODE
};

enum data_accuracy {
	ACCURACY_UNDEFINED = -1,
	ACCURACY_BAD = 0,
	ACCURACY_NORMAL =1,
	ACCURACY_GOOD = 2,
	ACCURACY_VERYGOOD = 3
};

enum data_unit_idx_t {
	IDX_UNDEFINED_UNIT,
	IDX_UNIT_METRE_PER_SECOND_SQUARED,
	IDX_UNIT_MICRO_TESLA,
	IDX_UNIT_DEGREE,
	IDX_UNIT_LUX,
	IDX_UNIT_CENTIMETER,
	IDX_UNIT_LEVEL_1_TO_10,
	IDX_UNIT_STATE_ON_OFF,
	IDX_UNIT_DEGREE_PER_SECOND,
	IDX_UNIT_HECTOPASCAL,
	IDX_UNIT_CELSIUS,
	IDX_UNIT_METER,
	IDX_UNIT_STEP,
	IDX_UNIT_VENDOR_UNIT = 100,
	IDX_UNIT_FILTER_CONVERTED,
};

enum sensor_id_t{
	ID_UNKNOWN,
	ID_ACCEL,
	ID_GEOMAG,
	ID_LUMINANT,
	ID_PROXI,
	ID_THERMER,
	ID_GYROSCOPE,
	ID_PRESSURE,
	ID_MOTION_ENGINE,
	ID_FUSION,
	ID_PEDO,
	ID_CONTEXT,
	ID_FLAT,
	ID_BIO,
	ID_BIO_HRM,
};

enum packet_type_t {
	CMD_NONE				= 0x00,
	CMD_GET_ID				= 0x01,
	CMD_HELLO				= 0x02,
	CMD_BYEBYE				= 0x03,
	CMD_WAIT_EVENT			= 0x04,
	CMD_DONE				= 0x05,
	CMD_START				= 0x06,
	CMD_STOP				= 0x07,
	CMD_REG					= 0x08,
	CMD_UNREG				= 0x09,
	CMD_CHECK_EVENT			= 0x0A,
	CMD_SET_OPTION			= 0x0B,
	CMD_SET_INTERVAL		= 0x0C,
	CMD_UNSET_INTERVAL		= 0x0D,
	CMD_SET_VALUE			= 0x0E,
	CMD_GET_PROPERTY 		= 0x0F,
	CMD_GET_STRUCT 			= 0x10,
	CMD_SEND_SENSORHUB_DATA = 0x11,
	CMD_LAST 				= 0x12,
	PROTOCOL_VERSION		= 0x100,
};

enum poll_value_t {
	POLL_100HZ		= 100,
	POLL_50HZ		= 50,
	POLL_25HZ		= 25,
	POLL_20HZ		= 20,
	POLL_10HZ		= 10,
	POLL_5HZ 		= 5,
	POLL_1HZ		= 1,
	POLL_100HZ_MS	= 10,
	POLL_50HZ_MS	= 20,
	POLL_25HZ_MS	= 40,
	POLL_20HZ_MS	= 50,
	POLL_10HZ_MS	= 100,
	POLL_5HZ_MS		= 200,
	POLL_1HZ_MS		= 1000,
	POLL_MAX_HZ_MS	= POLL_1HZ_MS,
};

typedef struct sensor_event_t {
	unsigned int event_type;
	int situation;
	unsigned long long timestamp;
	int data_accuracy;
	int data_unit_idx;
	int values_num;
	float values[MAX_VALUE_SIZE];
}sensor_event_t;

struct cmd_queue_item_t {
	cpacket *packet;
	csock::thread_arg_t *arg;
};

typedef struct {
	pid_t pid;
} cmd_get_id_t;

typedef struct {
	int client_id;
	int sensor;
} cmd_hello_t;

typedef struct {
	int client_id;
	int sensor;
} cmd_byebye_t;

typedef struct {
	int client_id;
	long value;
} cmd_done_t;

typedef struct {
	int client_id;
	unsigned int data_id;
} cmd_get_data_t;

typedef struct {
	int client_id;
	unsigned int property_level;
} cmd_get_property_t;

typedef struct {
	int sensor_unit_idx;
	float sensor_min_range;
	float sensor_max_range;
	float sensor_resolution;
	char sensor_name[MAX_KEY_LEN];
	char sensor_vendor[MAX_KEY_LEN];
} base_property_struct;

typedef struct {
	int data_accuracy;
	int data_unit_idx;
	unsigned long long time_stamp;
	int values_num;
	float values[MAX_VALUE_SIZE];
} base_data_struct;

typedef struct {
	int state;
	base_property_struct base_property;
} cmd_return_property_t;

typedef struct {
	int client_id;
	int state;
	base_data_struct base_data;
} cmd_get_struct_t;

typedef struct {
	int client_id;
	int sensor;
} cmd_start_t;

typedef struct {
	int client_id;
} cmd_stop_t;

typedef struct {
	int client_id;
	unsigned int event_type;
} cmd_reg_t;

typedef struct {
	int client_id;
	unsigned int event_type;
} cmd_unreg_t;

typedef struct {
	int client_id;
	unsigned int event_type;
} cmd_check_event_t;

typedef struct {
	int client_id;
	int sensor;
	unsigned int interval;
} cmd_set_interval_t;

typedef struct {
	int client_id;
	int sensor;
} cmd_unset_interval_t;

typedef struct {
	int client_id;
	int sensor;
	int option;
} cmd_set_option_t;

typedef struct  {
	int client_id;
	int sensor;
	unsigned int property;
	long value;
} cmd_set_value_t;

enum sensor_event_item_type {
	SENSOR_EVENT_ITEM = 0xCAFECAF1,
	SENSORHUB_EVENT_ITEM = 0xCAFECAF2,
};

typedef struct  {
	int client_id;
	int data_len;
	char data[0];
} cmd_send_sensorhub_data_t;

#define EVENT_CHANNEL_MAGIC 0xCAFECAFE

typedef struct {
	unsigned int magic;
	int client_id;
} event_channel_ready_t;


typedef void *(*cmd_func_t)(void *data, void *cb_data);

typedef std::vector<unsigned int> event_type_vector;



enum sensorhub_enable_bit {
	SENSORHUB_ACCELEROMETER_ENABLE_BIT = 0,
	SENSORHUB_GYROSCOPE_ENABLE_BIT,
	SENSORHUB_MAGNETIC_ENABLE_BIT,
	SENSORHUB_BAROMETER_ENABLE_BIT,
	SENSORHUB_GESTURE_ENABLE_BIT,
	SENSORHUB_PROXIMITY_ENABLE_BIT,
	SENSORHUB_TEMPERATURE_HUMIDITY_ENABLE_BIT,
	SENSORHUB_LIGHT_ENABLE_BIT,
	SENSORHUB_PROXIMITY_RAW_ENABLE_BIT,
	SENSORHUB_MAGNETIC_RAW_ENABLE_BIT,
	SENSORHUB_BIO_ENABLE_BIT = 18,
	SENSORHUB_BIO_HRM_ENABLE_BIT = 20,
	SENSORHUB_ENABLE_BIT_MAX,
};

#define SENSORHUB_TYPE_CONTEXT	1
#define SENSORHUB_TYPE_GESTURE	2

#define HUB_DATA_MAX_SIZE	4096

typedef struct sensorhub_event_t {
	unsigned int event_type;
	int situation;
	int version;
	int sensorhub;
	int type;
	int hub_data_size;
	long long timestamp;
	char hub_data[HUB_DATA_MAX_SIZE];
	float data[16];
} sensorhub_event_t;

#endif
