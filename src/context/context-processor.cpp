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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include <pthread.h>
#include <string.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <math.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <dirent.h>
#include <dlfcn.h>

#include <common.h>
#include <cobject_type.h>
#include <clist.h>
#include <cmutex.h>
#include <cmodule.h>
#include <cworker.h>
#include <cpacket.h>
#include <csock.h>
#include <sf_common.h>

#include <csensor_module.h>
#include <cprocessor_module.h>
#include <context-processor.h>

#include <vconf.h>
#include <algorithm>

using namespace std;

#define PROCESSOR_NAME "context_processor"

context_processor::context_processor()
: m_client(0)
, m_sensor(NULL)
, m_version(1)
, m_id(0x04BE)
, m_lcd_on_noti_id(0)
, m_lcd_off_noti_id(0)
, m_display_state(false)
, m_charger_state(false)
{
	m_name = PROCESSOR_NAME;

	register_supported_event(CONTEXT_EVENT_REPORT);

	cprocessor_module::set_main(working, stopped, this);
	INFO("context_processor is created!\n");

	start_listen_display_state();
}



context_processor::~context_processor()
{
	INFO("context_processor is destroyed!\n");

	stop_listen_display_state();
}


bool context_processor::add_input(csensor_module *sensor)
{
	m_sensor = sensor;
	return true;
}


bool context_processor::add_input(cprocessor_module *processor)
{
    return true;
}

const char *context_processor::name(void)
{
	return m_name.c_str();
}

int context_processor::id(void)
{
	return m_id;
}

int context_processor::version(void)
{
	return m_version;
}

int context_processor::get_processor_type(void)
{
	if (m_sensor)
		return m_sensor->get_sensor_type();

	return ID_CONTEXT;
}


bool context_processor::update_name(char *name)
{
	m_name = name;
	return true;
}



bool context_processor::update_id(int id)
{
	m_id = id;
	return true;
}



bool context_processor::update_version(int version)
{
	m_version = version;
	return true;
}



cprocessor_module *context_processor::create_new(void)
{
	return (cprocessor_module*)this;
}



void context_processor::destroy(cprocessor_module *module)
{
	bool bstate = false;

	bstate = cmodule::del_from_list((cmodule *)module);

	if (!bstate) {
		ERR("Destory and del_from_list fail");
		delete (context_processor *)module;
		return ;
	}
}

void *context_processor::stopped(void *inst)
{
	return (void*)NULL;
}

void *context_processor::working(void *inst)
{
	context_processor *processor = (context_processor*)inst;
	return (void*)(processor->process_event());
}

bool context_processor::start(void)
{
	bool ret;
	AUTOLOCK(m_mutex);

	if (m_client > 0) {
		m_client++;
		INFO("%s processor fake starting", m_name.c_str());
		return true;
	}

	INFO("%s processor real starting", m_name.c_str());

	ret = m_sensor ? m_sensor->start() : true;
	if ( ret != true ) {
		ERR("m_sensor start fail\n");
		return false;
	}

	initialize();

	cprocessor_module::start();
	m_client = 1;
	return ret;

}

bool context_processor::stop(void)
{
	bool ret;

	AUTOLOCK(m_mutex);
	if (m_client > 1) {
		m_client--;
		INFO("%s processor fake Stopping",m_name.c_str());
		return true;
	}

	INFO("%s processor real Stopping",m_name.c_str());

	ret = cprocessor_module::stop();
	if ( ret != true ) {
		ERR("cprocessor_module::stop()");
		return false;
	}

	finalize();

	ret = m_sensor ? m_sensor->stop() : true;
	if ( ret != true ) {
		ERR("m_sensor stop fail\n");
		return false;
	}

	m_client = 0;
	return ret;;
}

long context_processor::set_cmd(int type , int property , long input_value)
{
	int ret = 0;

	ret = m_sensor ? m_sensor->set_cmd(type, property, input_value) : 0;
	if (ret < 0) {
		ERR("m_sensor set_cmd fail");
		return ret;
	}

	DBG("ret = [%d] property = [%d]",ret, property);
	return ret;
}

int context_processor::get_property(unsigned int property_level , void *property_data )
{
	int result = -1;
	base_property_struct *return_property;
	return_property = (base_property_struct *)property_data;

	if (m_sensor) {
		result = m_sensor->get_property(CONTEXT_BASE_DATA_SET, return_property);
		if (result == 0)	{
			if (property_level == CONTEXT_BASE_DATA_SET) {
				return result;
			} else {
				ERR("cannot get_property from sensor");
				return -1;
			}

			return result;
		}
	} else {
		ERR("no m_sensor , cannot get_property from sensor");
	}

	return -1;
}

int context_processor::get_struct_value(unsigned int struct_type , void *struct_values)
{
	int state;
	sensorhub_event_t sensor_hub_event;
	sensorhub_event_t *return_sensor_hub_event = NULL;

	if (!m_sensor) {
		ERR("Sensor is not added");
		return -1;
	}

	state = m_sensor->get_struct_value(CONTEXT_BASE_DATA_SET , &sensor_hub_event);

	if (state < 0) {
		ERR("Failed to get_struct_value");
		return -1;
	}

	if (struct_type == CONTEXT_BASE_DATA_SET) {
		return_sensor_hub_event = (sensorhub_event_t *)struct_values;
		*return_sensor_hub_event = sensor_hub_event;
	} else {
		ERR("does not support struct_type");
		return -1;
	}

	return 0;

}

int context_processor::send_sensorhub_data(const char* data, int data_len)
{
	int ret;

	AUTOLOCK(m_command_lock);

	ret = send_sensorhub_data_internal(data, data_len);

	if ((ret < 0) && (ret != -EBUSY))
		return ret;

	if (ret == -EBUSY)
		WARN("Command is sent during sensorhub firmwate update");

	m_command_history.input_sensorhub_data(data, data_len);

	return 0;
}

bool context_processor::update_polling_interval(unsigned long interval)
{
	INFO("Polling interval is set to %dms", interval);

	return m_sensor->update_polling_interval(interval);
}

int context_processor::process_event(void)
{
	if (!m_sensor) {
		ERR("Sensor is not added\n");
		return cworker::STOPPED;
	}

	if (m_sensor->is_data_ready(true) == false) {
		return cworker::STARTED;
	}

	sensorhub_event_t sensorhub_event;
	m_sensor->get_struct_value(CONTEXT_BASE_DATA_SET, &sensorhub_event);

	{	/* the scope for the lock */
		AUTOLOCK(m_command_lock);

		if (is_reset_noti(sensorhub_event.hub_data, sensorhub_event.hub_data_size)) {
			ERR("Sensorhub MCU is resetted, reregister command!");
			reregister_command();
		}

		if (is_pedo_noti(sensorhub_event.hub_data, sensorhub_event.hub_data_size) && !is_display_on()) {
			INFO("Received Pedo data in LCD Off!");
			inform_current_time();
		}

	}

	AUTOLOCK(m_client_info_mutex);

	if (get_client_cnt(CONTEXT_EVENT_REPORT)) {
		sensorhub_event.timestamp = get_timestamp();
		sensorhub_event.event_type = CONTEXT_EVENT_REPORT;
		push(sensorhub_event);
	}

	return cworker::STARTED;

}

void context_processor::initialize(void)
{
	m_display_state = is_display_on();
	m_charger_state = is_charger_connected();

	DBG("Initial inform sensorhub of display: %s, charger: %s", m_display_state? "on" : "off",
		m_charger_state? "connected" : "disconnected");

	inform_display_state(m_display_state);
	inform_charger_state(m_charger_state);

	if (vconf_notify_key_changed(VCONFKEY_PM_STATE, display_state_cb, this) != 0)
		ERR("Fail to set notify callback for %s", VCONFKEY_PM_STATE);

	if (vconf_notify_key_changed(VCONFKEY_SYSMAN_CHARGER_STATUS, charger_state_cb, this) != 0)
		ERR("Fail to set notify callback for %s", VCONFKEY_SYSMAN_CHARGER_STATUS);

}
void context_processor::finalize(void)
{
	if (vconf_ignore_key_changed(VCONFKEY_PM_STATE, display_state_cb) < 0)
		ERR("Can't unset callback for %s", VCONFKEY_PM_STATE);

	if (vconf_ignore_key_changed(VCONFKEY_SYSMAN_CHARGER_STATUS, charger_state_cb) < 0)
		ERR("Can't unset callback for %s", VCONFKEY_SYSMAN_CHARGER_STATUS);


	m_command_history.clear();
}


int context_processor::send_sensorhub_data_internal(const char* data, int data_len)
{
	int state;

	if (!m_sensor) {
		ERR("Sensor is not added");
		return -1;
	}

	state = m_sensor->send_sensorhub_data(data, data_len);

	if (state < 0) {
		ERR("Failed to send_sensorhub_data");
		return state;
	}

	return 0;
}


bool context_processor::is_reset_noti(const char* data, int data_len)
{
	static const char reset_noti[] = {2, 1, (char)-43};

	if (data_len != sizeof(reset_noti))
		return false;

	return equal(reset_noti, reset_noti + sizeof(reset_noti), data);
}

bool context_processor::is_pedo_noti(const char* data, int data_len)
{
	static const char pedo_noti[] = {1, 1, 3};
	static const char pedo_noti_ext[] = {1, 3, 3};

	return (equal(pedo_noti, pedo_noti + sizeof(pedo_noti), data) || equal(pedo_noti_ext, pedo_noti_ext + sizeof(pedo_noti_ext), data));
}

void context_processor::reregister_command(void)
{
	vector<command> reg_commands;
	m_command_history.get_registered_commands(reg_commands);

	vector<command>::iterator it_command;

	it_command = reg_commands.begin();
	while (it_command != reg_commands.end()) {
		if (send_sensorhub_data_internal(it_command->data(), it_command->size()) < 0)
			ERR("Fail to reregister command");
		++it_command;
	}
}

void context_processor::display_signal_handler(GDBusConnection *conn, const gchar *name, const gchar *path, const gchar *interface,
		const gchar *sig, GVariant *param, gpointer user_data)
{
	context_processor* cp = static_cast<context_processor*>(user_data);

	if (!strcmp(sig, "LCDOn")) {
		INFO("Display state: LCD ON");
		cp->inform_display_state(true);
	} else if (!strcmp(sig, "LCDOff")) {
		INFO("Display state: LCD Off");
		cp->inform_display_state(false);
	}

}

bool context_processor::start_listen_display_state()
{
	GError *error = NULL;

	g_type_init();
	m_dbus_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	if (!m_dbus_conn) {
		ERR("Error creating dbus connection: %s\n", error->message);
		g_error_free (error);
		return false;
	}

	INFO("G-DBUS connected");

	m_lcd_on_noti_id = g_dbus_connection_signal_subscribe(m_dbus_conn,
		"org.tizen.system.deviced", /* sender */
		"org.tizen.system.deviced.display", /* Interface */
		"LCDOn", /* Member */
		"/Org/Tizen/System/DeviceD/Display", /* Object path */
		NULL, /* arg0 */
		G_DBUS_SIGNAL_FLAGS_NONE,
		display_signal_handler,
		this, NULL);

	INFO("noti_id for LCDOn [%d]", m_lcd_on_noti_id);

	m_lcd_off_noti_id = g_dbus_connection_signal_subscribe(m_dbus_conn,
		"org.tizen.system.deviced", /* sender */
		"org.tizen.system.deviced.display", /* Interface */
		"LCDOff", /* Member */
		"/Org/Tizen/System/DeviceD/Display", /* Object path */
		NULL, /* arg0 */
		G_DBUS_SIGNAL_FLAGS_NONE,
		display_signal_handler,
		this, NULL);

	INFO("noti_id for LCDOff [%d]", m_lcd_off_noti_id);

	return true;
}

bool context_processor::stop_listen_display_state()
{
	g_dbus_connection_signal_unsubscribe (m_dbus_conn, m_lcd_on_noti_id);
	g_dbus_connection_signal_unsubscribe (m_dbus_conn, m_lcd_off_noti_id);

	return true;

}

void context_processor::display_state_cb(keynode_t *node, void *user_data)
{
	bool display_state;

	context_processor* cp = static_cast<context_processor*>(user_data);

	display_state = cp->is_display_on();

	if (cp->m_display_state != display_state) {
		cp->m_display_state = display_state;

		if (!display_state)
			cp->inform_current_time();
	}
}


void context_processor::charger_state_cb(keynode_t *node, void *user_data)
{
	bool charger_state;
	context_processor* cp = static_cast<context_processor*>(user_data);

	charger_state = cp->is_charger_connected();

	if (cp->m_charger_state != charger_state) {
		cp->m_charger_state = charger_state;
		cp->inform_charger_state(charger_state);
	}
}

bool context_processor::is_display_on(void)
{
	int lcd_state;

	if (vconf_get_int(VCONFKEY_PM_STATE, &lcd_state) != 0) {
		ERR("Can't get the value of VCONFKEY_PM_STATE");
		return true;
	}

	if (lcd_state == VCONFKEY_PM_STATE_LCDOFF)
		return false;

	return true;
}


bool context_processor::inform_display_state(bool display_on)
{
	static const char display_on_noti_cmd[] = {(char)-76, 13, (char)-47, 0};
	static const char display_off_noti_cmd[] = {(char)-76, 13, (char)-46, 0};

	const char* cmd;
	int cmd_len;

	if (display_on) {
		cmd = display_on_noti_cmd;
		cmd_len = sizeof(display_on_noti_cmd);
		DBG("Inform sensorhub of display on");
	} else {
		cmd = display_off_noti_cmd;
		cmd_len = sizeof(display_off_noti_cmd);
		DBG("Inform sensorhub of display off");
	}

	return (send_sensorhub_data_internal(cmd, cmd_len) >= 0);
}

bool context_processor::is_charger_connected(void)
{
	int charger_state;

	if (vconf_get_int(VCONFKEY_SYSMAN_CHARGER_STATUS, &charger_state) != 0) {
		ERR("Can't get the value of VCONFKEY_SYSMAN_CHARGER_STATUS");
		return false;
	}

	return charger_state;
}

bool context_processor::inform_charger_state(bool connected)
{
	static const char charger_connected_noti_cmd[] = {(char)-76, 13, (char)-42, 0};
	static const char charger_disconnected_noti_cmd[] = {(char)-76, 13,(char)-41, 0};

	const char* cmd;
	int cmd_len;

	if (connected) {
		cmd = charger_connected_noti_cmd;
		cmd_len = sizeof(charger_connected_noti_cmd);
		DBG("Inform sensorhub of charger connected");
	} else {
		cmd = charger_disconnected_noti_cmd;
		cmd_len = sizeof(charger_disconnected_noti_cmd);
		DBG("Inform sensorhub of charger disconnected");
	}

	return (send_sensorhub_data_internal(cmd, cmd_len) >= 0);

}


bool context_processor::inform_current_time(void)
{
	char cureent_time_noti_cmd[] = {(char)-63, 14, 0, 0, 0};

	struct tm gm_time;
	time_t t;

	time(&t);
	gmtime_r(&t, &gm_time);

	cureent_time_noti_cmd[2] = gm_time.tm_hour;
	cureent_time_noti_cmd[3] = gm_time.tm_min;
	cureent_time_noti_cmd[4] = gm_time.tm_sec;

	return (send_sensorhub_data_internal(cureent_time_noti_cmd, sizeof(cureent_time_noti_cmd)) >= 0);
}

cmodule *module_init(void *win, void *egl)
{
	context_processor *inst;

	try {
		inst = new context_processor();
	} catch (int ErrNo) {
		ERR("context_processor class create fail , errno : %d , errstr : %s\n", ErrNo, strerror(ErrNo));
		return NULL;
	}

	return (cmodule*)inst;
}

void module_exit(cmodule *inst)
{
	context_processor *sample = (context_processor*)inst;
	delete sample;
}
