/*
 * sf-plugin-common
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

#if !defined(_ACCEL_PROCESSOR_CLASS_H_)
#define _ACCEL_PROCESSOR_CLASS_H_

#include <sensor.h>
#include <vconf.h>
#include <string>

using std::string;

#define MAX_WINDOW_NUM 2

class accel_processor : public cprocessor_module {
 public:
	static const char *LCD_TYPE_NODE;

	enum accel_value_t {
		LANDSCAPE		= 0x001,
		PORTRAIT_TOP	= 0x002,
		PORTRAIT_BTM	= 0x004,
		HEAD_LEFT		= 0x008,
		HEAD_CENTER		= 0x010,
		HEAD_RIGHT		= 0x020,
		FRONT			= 0x040,
		BACK			= 0x080,
		MIDDLE			= 0x100,
	};

	enum accel_rotation_event_t {
		ROTATION_UNKNOWN			= 0,
		ROTATION_HEAD_ROTATE_90		= 1,	/*CCW base */
		ROTATION_HEAD_ROTATE_0		= 2,	/*CCW base */
		ROTATION_HEAD_ROTATE_180	= 3,	/*CCW base */
		ROTATION_HEAD_ROTATE_270	= 4,	/*CCW base */
		ROTATION_LANDSCAPE_LEFT		= 1,
		ROTATION_PORTRAIT_TOP		= 2,
		ROTATION_PORTRAIT_BTM		= 3,
		ROTATION_LANDSCAPE_RIGHT	= 4,
	};

	enum accel_pitch_value_limit_t {
		PITCH_MIN = 35,		/*init 35 , 60 */
		PITCH_MAX = 145,	/*init 145 , 135 */
	};

	struct rotation_event {
		enum accel_rotation_event_t rotation;
		enum accel_rotation_event_t rm[2];
	};

	accel_processor();
	virtual ~ accel_processor();

	const char *name(void);
	int id(void);
	int version(void);
	int get_processor_type(void);

	bool update_name(char *name);
	bool update_id(int id);
	bool update_version(int version);

	bool add_input(csensor_module *sensor);
	bool add_input(cprocessor_module *processor);

	cprocessor_module *create_new(void);
	void destroy(cprocessor_module *module);

	static void *working(void *inst);
	static void *stopped(void *inst);

	virtual bool start(void);
	virtual bool stop(void);

	long set_cmd(int type, int property, long input_value);
	int get_property(unsigned int property_level, void *property_data);
	int get_struct_value(unsigned int struct_type, void *struct_values);

protected:
	bool update_polling_interval(unsigned long interval);

private:
	typedef void (*smart_detection_cb_t)(int eResult, void *user_data);
	typedef int (*get_smart_detection_t)(smart_detection_cb_t callback, int type, void *user_data1, void *user_data2);
	csensor_module *m_sensor;

	long m_x;
	long m_y;
	long m_z;

	float m_gravity_x;
	float m_gravity_y;
	float m_gravity_z;

	int m_rotation;
	int m_cur_rotation;
	int m_valid_rotation;

	long m_version;
	long m_id;

	string m_name;

	int m_client;
	int m_lcd_type;

	unsigned long long m_rotation_time;
	unsigned long m_polling_interval;
	int m_curr_window_count;

	int m_set_horizon_count;

	long m_windowing[MAX_WINDOW_NUM];
	struct rotation_event rotation_mode[5];

	void *m_smart_rotation_handle;
	bool m_smart_rotation_state;
	bool m_smart_rotation_progressing;
	int m_smart_rotation_wait_cnt;
	get_smart_detection_t m_get_smart_detection;

	bool m_pop_sync_rotation_mode;

	int m_rotation_check_interval;
	int m_rotation_check_remained_time;

	float m_raw_data_unit;

	cmutex m_mutex;
	cmutex m_value_mutex;

	bool add_client(unsigned int event_type);
	bool delete_client(unsigned int event_type);

	int get_rotation_event(float x, float y, float z);

	void raw_to_base(base_data_struct *raw, base_data_struct *base);
	void raw_to_orientation(base_data_struct *raw, base_data_struct *orientation);
	void raw_to_linear(base_data_struct *raw, base_data_struct *linear);
	void raw_to_gravity(base_data_struct *raw, base_data_struct *gravity);

	int check_lcd_type(void);
	bool is_smart_rotation_on(void);
	void reset_smart_rotation(void);
	void reset_auto_rotation(void);
	bool is_auto_rotation_time(void);

	bool load_smart_rotation(void);
	bool unload_smart_rotation(void);
	static void smart_rotation_enable_cb(keynode_t *node, void *data);
	static void smart_rotation_callback(int result, void *user_data);

	static void pop_sync_rotation_enable_cb(keynode_t *node, void *data);
	static void pop_sync_rotation_cb(keynode_t *node, void *data);
	void manual_rotate(int state);

	void set_rotation(int rotation);

	int process_event(void);
};

#endif
