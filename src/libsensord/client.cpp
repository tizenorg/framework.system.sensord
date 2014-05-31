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

#include <csock.h>
#include <sf_common.h>
#include <sensor.h>
#include <csensor_event_listener.h>
#include <client_common.h>
#include <vconf.h>
#include <cmutex.h>
#include <common.h>

#ifndef EXTAPI
#define EXTAPI __attribute__((visibility("default")))
#endif

static const int OP_SUCCESS = 0;
static const int OP_ERROR =  -1;
static const int CMD_ERROR = -2;

static csensor_event_listener event_listener;
static cmutex lock;

static bool g_power_save_state = false;

static bool get_power_save_state(void);
static void power_save_state_cb(keynode_t *node, void *data);
static void clean_up(void);
static void good_bye(void);
static int change_sensor_rep(sensor_type sensor, sensor_rep &prev_rep, sensor_rep &cur_rep);

void init_client(void)
{
	atexit(good_bye);
}

static void good_bye(void)
{
	_D("Good bye! %s\n", get_client_name());
	clean_up();
}

static int g_power_save_state_cb_cnt = 0;

static void set_power_save_state_cb(void)
{
	if (g_power_save_state_cb_cnt < 0)
		_E("g_power_save_state_cb_cnt(%d) is wrong", g_power_save_state_cb_cnt);

	++g_power_save_state_cb_cnt;

	if (g_power_save_state_cb_cnt == 1) {
		_D("Power save callback is registered");
		g_power_save_state = get_power_save_state();
		_D("power_save_state = [%s]", g_power_save_state ? "on" : "off");
		vconf_notify_key_changed(VCONFKEY_PM_STATE, power_save_state_cb, NULL);
	}
}

static void unset_power_save_state_cb(void)
{
	--g_power_save_state_cb_cnt;

	if (g_power_save_state_cb_cnt < 0)
		_E("g_power_save_state_cb_cnt(%d) is wrong", g_power_save_state_cb_cnt);

	if (g_power_save_state_cb_cnt == 0) {
		_D("Power save callback is unregistered");
		vconf_ignore_key_changed(VCONFKEY_PM_STATE, power_save_state_cb);
	}
}

static void clean_up(void)
{
	handle_vector handles;
	handle_vector::iterator it_handle;

	event_listener.get_all_handles(handles);

	it_handle = handles.begin();

	while (it_handle != handles.end()) {
		sf_disconnect(*it_handle);
		++it_handle;
	}
}


static bool get_power_save_state (void)
{
	int pm_state, ps_state;

	vconf_get_int(VCONFKEY_PM_STATE, &pm_state);

	if (pm_state == VCONFKEY_PM_STATE_LCDOFF)
		return true;

	return false;
}


static void power_save_state_cb(keynode_t *node, void *data)
{
	int val;
	bool cur_power_save_state;
	sensor_type_vector sensors;
	sensor_rep prev_rep, cur_rep;

	val = vconf_keynode_get_int(node);

	AUTOLOCK(lock);

	cur_power_save_state = get_power_save_state();

	if (cur_power_save_state == g_power_save_state) {
		_T("g_power_save_state NOT changed : [%d]", cur_power_save_state);
		return;
	}

	g_power_save_state = cur_power_save_state;
	_D("power_save_state %s noti to %s", g_power_save_state ? "on" : "off", get_client_name());

	event_listener.get_listening_sensors(sensors);

	sensor_type_vector::iterator it_sensor;

	it_sensor = sensors.begin();

	while (it_sensor != sensors.end()) {
		event_listener.get_sensor_rep(*it_sensor, prev_rep);

		if (cur_power_save_state)
			event_listener.pause_sensor(*it_sensor);
		else
			event_listener.resume_sensor(*it_sensor);

		event_listener.get_sensor_rep(*it_sensor, cur_rep);
		change_sensor_rep(*it_sensor, prev_rep, cur_rep);

		++it_sensor;
	}
}

static bool get_events_diff(event_type_vector &a_vec, event_type_vector &b_vec, event_type_vector &add_vec, event_type_vector &del_vec)
{
	sort(a_vec.begin(), a_vec.end());
	sort(b_vec.begin(), b_vec.end());

	set_difference(a_vec.begin(), a_vec.end(), b_vec.begin(), b_vec.end(), back_inserter(del_vec));
	set_difference(b_vec.begin(), b_vec.end(), a_vec.begin(), a_vec.end(), back_inserter(add_vec));

	return !(add_vec.empty() && del_vec.empty());
}


static int change_sensor_rep(sensor_type sensor, sensor_rep &prev_rep, sensor_rep &cur_rep)
{
	int client_id;
	command_channel *cmd_channel;
	event_type_vector add_event_types, del_event_types;

	if (!event_listener.get_command_channel(sensor, &cmd_channel)) {
		ERR("client %s failed to get command channel for %s", get_client_name(), get_sensor_name(sensor));
		return OP_ERROR;
	}

	client_id = event_listener.get_client_id();
	retvm_if ((client_id < 0), OP_ERROR, "Invalid client id : %d, %s, %s", client_id, get_sensor_name(sensor), get_client_name());

	get_events_diff(prev_rep.event_types, cur_rep.event_types, add_event_types, del_event_types);

	if (cur_rep.active) {
		if (prev_rep.option != cur_rep.option) {
			if (!cmd_channel->cmd_set_option(client_id, sensor, cur_rep.option)) {
				ERR("Sending cmd_set_option(%d, %s, %d) failed for %s", client_id, get_sensor_name(sensor), cur_rep.option, get_client_name());
				return CMD_ERROR;
			}
		}

		if (prev_rep.interval != cur_rep.interval) {
			unsigned int min_interval;

			if (cur_rep.interval == 0)
				min_interval= POLL_MAX_HZ_MS;
			else
				min_interval = cur_rep.interval;

			if (!cmd_channel->cmd_set_interval(client_id, sensor, min_interval)) {
				ERR("Sending cmd_set_interval(%d, %s, %d) failed for %s", client_id, get_sensor_name(sensor), min_interval, get_client_name());
				return CMD_ERROR;
			}
		}

		if (!add_event_types.empty()) {
			if (!cmd_channel->cmd_register_events(client_id, add_event_types)) {
				ERR("Sending cmd_register_events(%d, add_event_types) failed for %s", client_id, get_client_name());
				return CMD_ERROR;
			}
		}

	}

	if (prev_rep.active && !del_event_types.empty()) {
		if (!cmd_channel->cmd_unregister_events(client_id, del_event_types)) {
			ERR("Sending cmd_unregister_events(%d, del_event_types) failed for %s", client_id, get_client_name());
			return CMD_ERROR;
		}
	}

	if (prev_rep.active != cur_rep.active) {
		if (cur_rep.active) {
			if (!cmd_channel->cmd_start(client_id, sensor)) {
				ERR("Sending cmd_start(%d, %s) failed for %s", client_id, get_sensor_name(sensor), get_client_name());
				return CMD_ERROR;
			}
		} else {
			if (!cmd_channel->cmd_unset_interval(client_id, sensor)) {
				ERR("Sending cmd_unset_interval(%d, %s) failed for %s", client_id, get_sensor_name(sensor), get_client_name());
				return CMD_ERROR;
			}

			if (!cmd_channel->cmd_stop(client_id, sensor)) {
				ERR("Sending cmd_stop(%d, %s) failed for %s", client_id, get_sensor_name(sensor), get_client_name());
				return CMD_ERROR;
			}
		}
	}

	return OP_SUCCESS;
}

EXTAPI int sf_connect(sensor_type sensor)
{
	command_channel *cmd_channel = NULL;
	int handle;
	int client_id;
	bool sensor_registered;
	bool first_connection = false;

	if (!is_sensor_supported(sensor)) {
		ERR("Undefined sensor: 0x%x for client %s", sensor, get_client_name());
		return OP_ERROR;
	}

	AUTOLOCK(lock);

	sensor_registered = event_listener.is_sensor_registered(sensor);

	handle = event_listener.create_handle(sensor);
	if (handle == MAX_HANDLE) {
		ERR("Maximum number of handles reached, sensor: %s in client %s", get_sensor_name(sensor), get_client_name());
		return OP_ERROR;
	}

	if (!sensor_registered) {
		cmd_channel = new command_channel();

		if (!cmd_channel->create_channel()) {
			ERR("%s failed to create command channel for %s", get_client_name(), get_sensor_name(sensor));
			event_listener.delete_handle(handle);
			return OP_ERROR;
		}

		event_listener.set_command_channel(sensor, cmd_channel);
	}

	if (!event_listener.get_command_channel(sensor, &cmd_channel)) {
		ERR("%s failed to get command channel for %s", get_client_name(), get_sensor_name(sensor));
		event_listener.delete_handle(handle);
		return OP_ERROR;
	}

	if (!event_listener.has_client_id()) {
		first_connection = true;
		if(!cmd_channel->cmd_get_id(client_id)) {
			ERR("Sending cmd_get_id() failed for %s", get_sensor_name(sensor));
			event_listener.close_command_channel(sensor);
			event_listener.delete_handle(handle);
			return CMD_ERROR;
		}

		event_listener.set_client_id(client_id);
		INFO("%s gets client_id [%d]", get_client_name(), client_id);
		event_listener.start_event_listener();
		INFO("%s starts listening events with client_id [%d]", get_client_name(), client_id);
	}

	client_id = event_listener.get_client_id();

	INFO("%s[%d] connects with %s[%d]", get_client_name(), client_id, get_sensor_name(sensor), handle);

	event_listener.set_sensor_params(handle, SENSOR_STATE_STOPPED, SENSOR_OPTION_DEFAULT);

	if (!sensor_registered) {
		if(!cmd_channel->cmd_hello(client_id, sensor)) {
			ERR("Sending cmd_hello(%s, %d) failed for %s", get_sensor_name(sensor), client_id, get_client_name());
			event_listener.close_command_channel(sensor);
			event_listener.delete_handle(handle);
			if (first_connection)
				event_listener.stop_event_listener();

			return CMD_ERROR;
		}
	}

	set_power_save_state_cb();
	return handle;
}

EXTAPI int sf_disconnect(int handle)
{
	command_channel *cmd_channel;
	sensor_type sensor;
	int client_id;
	int sensor_state;

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_state(handle, sensor_state)||
		!event_listener.get_sensor_type(handle, sensor)) {
		ERR("client %s failed to get handle information", get_client_name());
		return OP_ERROR;
	}

	if (!event_listener.get_command_channel(sensor, &cmd_channel)) {
		ERR("client %s failed to get command channel for %s", get_client_name(), get_sensor_name(sensor));
		return OP_ERROR;
	}

	client_id = event_listener.get_client_id();
	retvm_if ((client_id < 0), OP_ERROR, "Invalid client id : %d, handle: %d, %s, %s", client_id, handle, get_sensor_name(sensor), get_client_name());

	INFO("%s disconnects with %s[%d]", get_client_name(), get_sensor_name(sensor), handle);

	if (sensor_state != SENSOR_STATE_STOPPED) {
		WARN("Before disconnecting, sensor %s[%d] is forced to stop in client %s",
			get_sensor_name(sensor), handle, get_client_name());
		sf_stop(handle);
	}

	if (!event_listener.delete_handle(handle))
		return OP_ERROR;

	if (!event_listener.is_sensor_registered(sensor)) {
		if(!cmd_channel->cmd_byebye(client_id, sensor)) {
			ERR("Sending cmd_byebye(%d, %s) failed for %s", client_id, get_sensor_name(sensor), get_client_name());
			return CMD_ERROR;
		}
		event_listener.close_command_channel(sensor);
	}

	if (!event_listener.is_active()) {
		INFO("Stop listening events for client %s with client id [%d]", get_client_name(), event_listener.get_client_id());
		event_listener.stop_event_listener();
	}

	unset_power_save_state_cb();

	return OP_SUCCESS;
}

EXTAPI int sf_start(int handle, int option)
{
	sensor_type sensor;
	sensor_rep prev_rep, cur_rep;

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_type(handle, sensor)) {
		ERR("client %s failed to get handle information", get_client_name());
		return OP_ERROR;
	}

	retvm_if ((option < 0) || (option >= SENSOR_OPTION_END), OP_ERROR, "Invalid option value : %d, handle: %d, %s, %s",
		option, handle, get_sensor_name(sensor), get_client_name());

	INFO("%s starts %s[%d], with option: %d%s", get_client_name(), get_sensor_name(sensor),
		handle, option, g_power_save_state ? " in power save state" : "");

	if (g_power_save_state && (option != SENSOR_OPTION_ALWAYS_ON)) {
		event_listener.set_sensor_params(handle, SENSOR_STATE_PAUSED, option);
		return OP_SUCCESS;
	}

	event_listener.get_sensor_rep(sensor, prev_rep);
	event_listener.set_sensor_params(handle, SENSOR_STATE_STARTED, option);
	event_listener.get_sensor_rep(sensor, cur_rep);

	return change_sensor_rep(sensor, prev_rep, cur_rep);

}

EXTAPI int sf_stop(int handle)
{
	sensor_type sensor;
	int sensor_state;

	sensor_rep prev_rep, cur_rep;

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_state(handle, sensor_state)||
		!event_listener.get_sensor_type(handle, sensor)) {
		ERR("client %s failed to get handle information", get_client_name());
		return OP_ERROR;
	}

	retvm_if ((sensor_state == SENSOR_STATE_STOPPED), OP_SUCCESS, "%s already stopped with %s[%d]",
		get_client_name(), get_sensor_name(sensor), handle);


	INFO("%s stops sensor %s[%d]", get_client_name(), get_sensor_name(sensor), handle);

	event_listener.get_sensor_rep(sensor, prev_rep);
	event_listener.set_sensor_state(handle, SENSOR_STATE_STOPPED);
	event_listener.get_sensor_rep(sensor, cur_rep);

	return change_sensor_rep(sensor, prev_rep, cur_rep);
}

EXTAPI int sf_register_event(int handle, unsigned int event_type, event_condition_t *event_condition, sensor_callback_func_t cb, void *cb_data )
{
	sensor_type sensor;
	unsigned int interval = BASE_GATHERING_INTERVAL;
	sensor_rep prev_rep, cur_rep;

	retvm_if ((cb  == NULL), OP_ERROR, "callback is NULL");

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_type(handle, sensor)) {
		ERR("client %s failed to get handle information", get_client_name());
		return OP_ERROR;
	}

	if (event_condition != NULL) {
		if ((event_condition->cond_op == CONDITION_EQUAL) && (event_condition->cond_value1 > 0))
			interval = event_condition->cond_value1;
	}

	INFO("%s registers event %s[0x%x] for sensor %s[%d] with interval: %d, cb: 0x%x, cb_data: 0x%x", get_client_name(), get_event_name(event_type),
			event_type, get_sensor_name(sensor), handle, interval, cb, cb_data);


	event_listener.get_sensor_rep(sensor, prev_rep);
	event_listener.register_event(handle, event_type, interval, cb, cb_data);
	event_listener.get_sensor_rep(sensor, cur_rep);
	return change_sensor_rep(sensor, prev_rep, cur_rep);

}

EXTAPI int sf_unregister_event(int handle, unsigned int event_type)
{
	sensor_type sensor;
	sensor_rep prev_rep, cur_rep;

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_type(handle, sensor)) {
		ERR("client %s failed to get handle information", get_client_name());
		return OP_ERROR;
	}

	INFO("%s unregisters event %s[0x%x] for sensor %s[%d]", get_client_name(), get_event_name(event_type),
		event_type, get_sensor_name(sensor), handle);

	event_listener.get_sensor_rep(sensor, prev_rep);
	if (!event_listener.unregister_event(handle, event_type)) {
		ERR("%s try to unregister non registered event %s[0x%x] for sensor %s[%d]",
			get_client_name(),get_event_name(event_type), event_type, get_sensor_name(sensor), handle);
		return OP_ERROR;
	}
	event_listener.get_sensor_rep(sensor, cur_rep);
	return change_sensor_rep(sensor, prev_rep, cur_rep);

}

EXTAPI int sf_change_event_condition(int handle, unsigned int event_type, event_condition_t *event_condition)
{
	sensor_type sensor;
	sensor_rep prev_rep, cur_rep;

	unsigned int interval = BASE_GATHERING_INTERVAL;

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_type(handle, sensor)) {
		ERR("client %s failed to get handle information", get_client_name());
		return OP_ERROR;
	}

	if (event_condition != NULL) {
		if ((event_condition->cond_op == CONDITION_EQUAL) && (event_condition->cond_value1 > 0))
			interval = event_condition->cond_value1;
	}

	INFO("%s changes interval of event %s[0x%x] for %s[%d] to interval %d", get_client_name(), get_event_name(event_type),
			event_type, get_sensor_name(sensor), handle, interval);

	event_listener.get_sensor_rep(sensor, prev_rep);
	event_listener.set_event_interval(handle, event_type, interval);
	event_listener.get_sensor_rep(sensor, cur_rep);
	return change_sensor_rep(sensor, prev_rep, cur_rep);

}


int sf_change_sensor_option(int handle, int option)
{
	sensor_type sensor;
	sensor_rep prev_rep, cur_rep;
	int sensor_state;

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_state(handle, sensor_state)||
		!event_listener.get_sensor_type(handle, sensor)) {
		ERR("client %s failed to get handle information", get_client_name());
		return OP_ERROR;
	}

	retvm_if ((option < 0) || (option >= SENSOR_OPTION_END), OP_ERROR, "Invalid option value : %d, handle: %d, %s, %s",
		option, handle, get_sensor_name(sensor), get_client_name());


	event_listener.get_sensor_rep(sensor, prev_rep);

	if (g_power_save_state) {
		if ((option == SENSOR_OPTION_ALWAYS_ON) && (sensor_state == SENSOR_STATE_PAUSED))
			event_listener.set_sensor_state(handle, SENSOR_STATE_STARTED);
		else if ((option == SENSOR_OPTION_DEFAULT) && (sensor_state == SENSOR_STATE_STARTED))
			event_listener.set_sensor_state(handle, SENSOR_STATE_PAUSED);
	}
	event_listener.set_sensor_option(handle, option);

	event_listener.get_sensor_rep(sensor, cur_rep);
	return change_sensor_rep(sensor, prev_rep, cur_rep);
}


EXTAPI int sf_send_sensorhub_data(int handle, const char* data, int data_len)
{
	sensor_type sensor;
	command_channel *cmd_channel;
	int client_id;

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_type(handle, sensor)) {
		ERR("client %s failed to get handle information", get_client_name());
		return OP_ERROR;
	}

	retvm_if (sensor != CONTEXT_SENSOR, OP_ERROR, "%s use this API wrongly, only for CONTEXT_SENSOR not for %s",
		get_client_name(), get_sensor_name(sensor));

	if (!event_listener.get_command_channel(sensor, &cmd_channel)) {
		ERR("client %s failed to get command channel for %s", get_client_name(), get_sensor_name(sensor));
		return OP_ERROR;
	}

	retvm_if((data_len < 0) || (data == NULL), OP_ERROR, "Invalid data_len: %d, data: 0x%x, handle: %d, %s, %s",
		data_len, data, handle, get_sensor_name(sensor), get_client_name());

	client_id = event_listener.get_client_id();
	retvm_if ((client_id < 0), OP_ERROR, "Invalid client id : %d, handle: %d, %s, %s", client_id, handle, get_sensor_name(sensor), get_client_name());

	retvm_if (!event_listener.is_sensor_active(sensor), OP_ERROR, "%s with client_id:%d is not active state for %s with handle: %d",
		get_sensor_name(sensor), client_id, get_client_name(), handle);

	if (!cmd_channel->cmd_send_sensorhub_data(client_id, data_len, data)) {
		ERR("Sending cmd_send_sensorhub_data(%d, %d, 0x%x) failed for %s",
			client_id, data_len, data, get_client_name);
		return CMD_ERROR;
	}

	return OP_SUCCESS;
}


EXTAPI int sf_is_sensor_event_available (sensor_type sensor, unsigned int event)
{
	command_channel *cmd_channel;
	int handle;
	int client_id;

	handle = sf_connect(sensor);
	if (handle < 0) {
		return OP_ERROR;
	}

	AUTOLOCK(lock);

	INFO("%s checks if event %s[0x%x] is registered", get_client_name(), get_event_name(event), event);

	if (!event_listener.get_command_channel(sensor, &cmd_channel)) {
		ERR("client %s failed to get command channel for %s", get_client_name(), get_sensor_name(sensor));
		return OP_ERROR;
	}

	client_id = event_listener.get_client_id();
	retvm_if ((client_id < 0), OP_ERROR, "Invalid client id : %d, handle: %d, %s, %s", client_id, handle, get_sensor_name(sensor), get_client_name());

	if(!cmd_channel->cmd_check_event(client_id, event)) {
		INFO("Sensor event %s is not supported in sensor %s", get_event_name(event), get_sensor_name(sensor));
		return CMD_ERROR;
	}

	sf_disconnect(handle);

	return OP_SUCCESS;
}

static int server_get_properties(int handle, unsigned int data_id, void *property_data)
{
	command_channel *cmd_channel;
	sensor_type sensor;
	int client_id;

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_type(handle, sensor)) {
		ERR("client %s failed to get handle information", get_client_name());
		return OP_ERROR;
	}

	if (!event_listener.get_command_channel(sensor, &cmd_channel)) {
		ERR("client %s failed to get command channel for %s", get_client_name(), get_sensor_name(sensor));
		return OP_ERROR;
	}

	client_id = event_listener.get_client_id();
	retvm_if ((client_id < 0), OP_ERROR, "Invalid client id : %d, handle: %d, %s, %s", client_id, handle, get_sensor_name(sensor), get_client_name());

	INFO("%s gets property with %s[%d], property_id: %d", get_client_name(), get_sensor_name(sensor), handle, data_id);

	if(!cmd_channel->cmd_get_property(client_id, sensor, data_id, property_data)) {
		ERR("Sending cmd_get_property(%d, %s, %d, 0x%x) failed for %s", client_id, get_sensor_name(sensor), data_id, property_data, get_client_name());
		return CMD_ERROR;
	}

	return OP_SUCCESS;
}

static int server_set_property(int handle, unsigned int property_id, long value)
{
	command_channel *cmd_channel;
	sensor_type sensor;
	int client_id;

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_type(handle, sensor)) {
		ERR("client %s failed to get handle information", get_client_name());
		return OP_ERROR;
	}

	if (!event_listener.get_command_channel(sensor, &cmd_channel)) {
		ERR("client %s failed to get command channel for %s", get_client_name(), get_sensor_name(sensor));
		return OP_ERROR;
	}


	client_id = event_listener.get_client_id();
	retvm_if ((client_id < 0), OP_ERROR, "Invalid client id : %d, handle: %d, %s, %s", client_id, handle, get_sensor_name(sensor), get_client_name());

	INFO("%s gets property with %s[%d], property_id: %d", get_client_name(), get_sensor_name(sensor), handle, property_id);

	if(!cmd_channel->cmd_set_value(client_id, sensor, property_id, value)) {
		ERR("Cmd_set_value(%d, %s, %d, %d) failed for %s", client_id, get_sensor_name(sensor), property_id, value, get_client_name());
		return CMD_ERROR;
	}

	return OP_SUCCESS;;
}

EXTAPI int sf_get_data_properties(unsigned int data_id, sensor_data_properties_t *return_data_properties)
{
	int handle;
	int state = -1;

	retvm_if ((!return_data_properties ), -1, "Invalid return properties pointer : %p from %s",
			return_data_properties, get_client_name());

	handle = sf_connect((sensor_type)(data_id >> 16));

	if (handle < 0) {
		ERR("Sensor connet fail !! for : %x \n", (data_id >> 16));
		return OP_ERROR;
	} else {
		state = server_get_properties(handle, data_id, return_data_properties );
		if (state < 0)
			ERR("server_get_properties fail, state : %d \n", state);

		sf_disconnect(handle);
	}

	return state;
}


static unsigned int get_sensor_property_level(sensor_type sensor)
{
	const int SENSOR_TYPE_SHIFT = 16;
	return (sensor << SENSOR_TYPE_SHIFT) | 0x0001;
}

EXTAPI int sf_get_properties(sensor_type sensor, sensor_properties_t *return_properties)
{
	int handle;
	int state = -1;

	retvm_if ((!return_properties ), -1, "Invalid return properties pointer : %p from %s",
			return_properties, get_client_name());

	handle = sf_connect(sensor);

	if (handle < 0) {
		ERR("Sensor connet fail !! for : %x \n", sensor);
		return OP_ERROR;
	} else {
		state = server_get_properties(handle, get_sensor_property_level(sensor), return_properties );
		if (state < 0)
			ERR("server_get_properties fail, state : %d \n", state);
		sf_disconnect(handle);
	}

	return state;
}


EXTAPI int sf_set_property(sensor_type sensor, unsigned int property_id, long value)
{
	int handle;
	int state = -1;

	handle = sf_connect(sensor);

	if (handle < 0) {
		ERR("Sensor connet fail !! for : %x \n", sensor);
		return OP_ERROR;
	} else {
		state = server_set_property(handle, property_id, value );
		if (state < 0)
			ERR("server_set_property fail, state : %d \n", state);

		sf_disconnect(handle);
	}

	return state;
}



EXTAPI int sf_get_data(int handle, unsigned int data_id, sensor_data_t* sensor_data)
{
	sensor_type sensor;
	command_channel *cmd_channel;
	int sensor_state;
	int client_id;

	retvm_if ((!sensor_data),OP_ERROR, "sf_get_data fail, invalid get_values pointer %p", sensor_data);

	AUTOLOCK(lock);

	if (!event_listener.get_sensor_state(handle, sensor_state)||
		!event_listener.get_sensor_type(handle, sensor)) {
		ERR("client %s failed to get handle information", get_client_name());
		return OP_ERROR;
	}

	if (!event_listener.get_command_channel(sensor, &cmd_channel)) {
		ERR("client %s failed to get command channel for %s", get_client_name(), get_sensor_name(sensor));
		return OP_ERROR;
	}


	client_id = event_listener.get_client_id();
	retvm_if ((client_id < 0), OP_ERROR, "Invalid client id : %d, handle: %d, %s, %s", client_id, handle, get_sensor_name(sensor), get_client_name());

	if (sensor_state != SENSOR_STATE_STARTED) {
		ERR("Sensor %s is not started for client %s with handle: %d, sensor_state: %d", get_sensor_name(sensor), get_client_name(), handle, sensor_state);
		return OP_ERROR;
	}

	if(!cmd_channel->cmd_get_struct(client_id, data_id, sensor_data)) {
		ERR("Cmd_get_struct(%d, %d, 0x%x) failed for %s", client_id, data_id, sensor_data, get_client_name());
		return CMD_ERROR;
	}

	return OP_SUCCESS;
}

EXTAPI int sf_check_rotation(unsigned long *curr_state)
{
	int state = -1;
	int handle = 0;

	sensor_data_t base_data_values;

	retvm_if (curr_state==NULL, -1, "sf_check_rotation fail, invalid curr_state");

	*curr_state = ROTATION_UNKNOWN;

	handle = sf_connect(ACCELEROMETER_SENSOR);
	if (handle<0) {
		ERR("sensor attach fail\n");
		return OP_ERROR;
	}

	state = sf_start(handle, 1);
	if (state < 0) {
		ERR("sf_start fail\n");
		return OP_ERROR;
	}

	state = sf_get_data(handle, ACCELEROMETER_ROTATION_DATA_SET, &base_data_values);
	if (state < 0) {
		ERR("sf_get_data fail\n");
		return OP_ERROR;
	}

	state = sf_stop(handle);
	if (state < 0) {
		ERR("sf_stop fail\n");
		return OP_ERROR;
	}

	state = sf_disconnect(handle);
	if (state < 0) {
		ERR("sf_disconnect fail\n");
		return OP_ERROR;
	}

	*curr_state = base_data_values.values[0];

	INFO("%s gets %s by checking rotation\n", get_client_name(), get_rotate_name(*curr_state));

	return OP_SUCCESS;
}
