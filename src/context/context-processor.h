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

#if !defined(_CONTEXT_PROCESSOR_CLASS_H_)
#define _CONTEXT_PROCESSOR_CLASS_H_

#include <string>
#include <vconf.h>
#include <sensor.h>
#include <command_history.h>
#include <glib.h>
#include <gio/gio.h>

using std::string;

class context_processor : public cprocessor_module {
 public:
	context_processor();
	virtual ~ context_processor();

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

	int send_sensorhub_data(const char* data, int data_len);
protected:
	bool update_polling_interval(unsigned long interval);

private:
	csensor_module *m_sensor;

	int m_client;
	long m_version;
	long m_id;

	GDBusConnection *m_dbus_conn;
	unsigned int m_lcd_on_noti_id;
	unsigned int m_lcd_off_noti_id;

	string m_name;

	cmutex m_mutex;
	cmutex m_command_lock;
	command_history m_command_history;

	bool m_display_state;
	bool m_charger_state;

	int process_event(void);

	void initialize(void);
	void finalize(void);

	int send_sensorhub_data_internal(const char* data, int data_len);

	bool is_reset_noti(const char* data, int data_len);
	bool is_pedo_noti(const char* data, int data_len);

	void reregister_command(void);

	bool is_display_on(void);
	bool inform_display_state(bool display_on);
	bool is_charger_connected(void);
	bool inform_charger_state(bool connected);
	bool inform_current_time(void);

	static void display_signal_handler(GDBusConnection *conn, const gchar *name, const gchar *path, const gchar *interface,
			const gchar *sig, GVariant *param, gpointer user_data);
	bool start_listen_display_state();
	bool stop_listen_display_state();

	static void display_state_cb(keynode_t *node, void *user_data);
	static void charger_state_cb(keynode_t *node, void *user_data);
};

#endif
