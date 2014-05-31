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

#include <csensor_event_listener.h>
#include <client_common.h>
#include <sf_common.h>

#include <thread>
#include <chrono>

using std::thread;
using std::pair;



csensor_event_listener::csensor_event_listener()
: m_client_id(CLIENT_ID_INVALID)
, m_thread_state(THREAD_STATE_TERMINATE)
, m_poller(NULL)
{
}

csensor_event_listener::~csensor_event_listener()
{
	stop_event_listener();
}

int csensor_event_listener::create_handle(const sensor_type sensor)
{
	csensor_handle_info handle_info;
	int handle = 0;

	AUTOLOCK(m_handle_info_lock);

	while (m_sensor_handle_infos.count(handle) > 0)
		handle++;

	if (handle == MAX_HANDLE) {
		ERR("Handles of client %s are full", get_client_name());
		return MAX_HANDLE_REACHED;
	}

	handle_info.m_sensor_type = sensor;
	handle_info.m_sensor_state = SENSOR_STATE_STOPPED;
	handle_info.m_sensor_option = SENSOR_OPTION_DEFAULT;
	handle_info.m_handle = handle;

	m_sensor_handle_infos.insert(pair<int,csensor_handle_info> (handle, handle_info));

	return handle;
}

bool csensor_event_listener::delete_handle(const int handle)
{
	sensor_handle_info_map::iterator it_handle;

	AUTOLOCK(m_handle_info_lock);

	it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	m_sensor_handle_infos.erase(it_handle);
	return true;
}


bool csensor_event_listener::is_active()
{
	AUTOLOCK(m_handle_info_lock);

	return !m_sensor_handle_infos.empty();
}

bool csensor_event_listener::start_handle(const int handle)
{
	return set_sensor_state(handle, SENSOR_STATE_STARTED);
}

bool csensor_event_listener::stop_handle(const int handle)
{
	return set_sensor_state(handle, SENSOR_STATE_STOPPED);
}

bool csensor_event_listener::register_event(const int handle, const unsigned int event_type,
		const unsigned int interval, const sensor_callback_func_t callback, void* cb_data)
{
	sensor_handle_info_map::iterator it_handle;

	AUTOLOCK(m_handle_info_lock);

	it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	if (!it_handle->second.add_reg_event_info(event_type, interval, callback, cb_data))
		return false;

	return true;
}

bool csensor_event_listener::unregister_event(const int handle, const unsigned int event_type)
{
	sensor_handle_info_map::iterator it_handle;

	AUTOLOCK(m_handle_info_lock);

	it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	if (!it_handle->second.delete_reg_event_info(event_type))
		return false;

	return true;
}

int csensor_event_listener::check_event_registered(const sensor_type sensor, const unsigned int event_type)
{
	event_type_vector event_types;

	AUTOLOCK(m_handle_info_lock);

	sensor_handle_info_map::iterator it_handle;

	it_handle = m_sensor_handle_infos.begin();
	while (it_handle != m_sensor_handle_infos.end()) {
		if (it_handle->second.m_sensor_type == sensor) {
			it_handle->second.get_reg_event_types(event_types);
			if (find (event_types.begin(), event_types.end(), event_type) != event_types.end())
				return it_handle->first;
		}
		++it_handle;
	}

	return -1;
}

bool csensor_event_listener::change_event_interval(const int handle, const unsigned int event_type,
		const unsigned int interval)
{
	sensor_handle_info_map::iterator it_handle;

	AUTOLOCK(m_handle_info_lock);

	it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	if (!it_handle->second.change_reg_event_interval(event_type, interval))
		return false;

	return true;
}


bool csensor_event_listener::set_sensor_params(const int handle, int sensor_state, int sensor_option)
{
	sensor_handle_info_map::iterator it_handle;

	AUTOLOCK(m_handle_info_lock);

	it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	it_handle->second.m_sensor_state = sensor_state;
	it_handle->second.m_sensor_option = sensor_option;

	return true;
}

bool csensor_event_listener::set_sensor_state(const int handle, const int sensor_state)
{
	sensor_handle_info_map::iterator it_handle;

	AUTOLOCK(m_handle_info_lock);

	it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	it_handle->second.m_sensor_state = sensor_state;

	return true;
}

bool csensor_event_listener::set_sensor_option(const int handle, const int sensor_option)
{
	sensor_handle_info_map::iterator it_handle;

	AUTOLOCK(m_handle_info_lock);

	it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	it_handle->second.m_sensor_option = sensor_option;

	return true;
}

bool csensor_event_listener::set_event_interval(const int handle, const unsigned int event_type, const unsigned int interval)
{
	sensor_handle_info_map::iterator it_handle;

	AUTOLOCK(m_handle_info_lock);

	it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	if (!it_handle->second.change_reg_event_interval(event_type, interval))
		return false;

	return true;
}

void csensor_event_listener::get_listening_sensors(sensor_type_vector &sensors)
{
	sensor_handle_info_map::iterator it_handle;

	AUTOLOCK(m_handle_info_lock);

	it_handle = m_sensor_handle_infos.begin();

	while (it_handle != m_sensor_handle_infos.end()) {
		sensors.push_back(it_handle->second.m_sensor_type);
		++it_handle;
	}

	sort(sensors.begin(), sensors.end());
	unique(sensors.begin(),sensors.end());
}


void csensor_event_listener::get_sensor_rep(sensor_type sensor, sensor_rep& rep)
{
	AUTOLOCK(m_handle_info_lock);

	rep.active = is_sensor_active(sensor);
	rep.option = get_active_option(sensor);
	rep.interval = get_active_min_interval(sensor);
	get_active_event_types(sensor, rep.event_types);

}

void csensor_event_listener::pause_sensor(const sensor_type sensor)
{
	sensor_handle_info_map::iterator it_handle;

	AUTOLOCK(m_handle_info_lock);

	it_handle = m_sensor_handle_infos.begin();

	while (it_handle != m_sensor_handle_infos.end()) {
		if ((it_handle->second.m_sensor_type == sensor) &&
			(it_handle->second.m_sensor_state == SENSOR_STATE_STARTED) &&
			(it_handle->second.m_sensor_option != SENSOR_OPTION_ALWAYS_ON)) {
				it_handle->second.m_sensor_state = SENSOR_STATE_PAUSED;
				INFO("%s's %s[%d] is paused", get_client_name(), get_sensor_name(sensor), it_handle->first);
			}

		++it_handle;
	}
}

void csensor_event_listener::resume_sensor(const sensor_type sensor)
{
	sensor_handle_info_map::iterator it_handle;

	AUTOLOCK(m_handle_info_lock);

	it_handle = m_sensor_handle_infos.begin();

	while (it_handle != m_sensor_handle_infos.end()) {
		if ((it_handle->second.m_sensor_type == sensor) &&
			(it_handle->second.m_sensor_state == SENSOR_STATE_PAUSED)) {
				it_handle->second.m_sensor_state = SENSOR_STATE_STARTED;
				INFO("%s's %s[%d] is resumed", get_client_name(), get_sensor_name(sensor), it_handle->first);
		}

		++it_handle;
	}

}

bool csensor_event_listener::set_command_channel(sensor_type sensor, command_channel *cmd_channel)
{
	sensor_command_channel_map::iterator it_channel;

	it_channel = m_command_channels.find(sensor);

	if (it_channel != m_command_channels.end()) {
		ERR("%s alreay has command_channel for %s", get_client_name(), get_sensor_name(sensor));
		return false;
	}

	m_command_channels.insert(pair<sensor_type, command_channel *> (sensor, cmd_channel));

	return true;

}
bool csensor_event_listener::get_command_channel(sensor_type sensor, command_channel **cmd_channel)
{
	sensor_command_channel_map::iterator it_channel;

	it_channel = m_command_channels.find(sensor);

	if (it_channel == m_command_channels.end()) {
		ERR("%s doesn't have command_channel for %s", get_client_name(), get_sensor_name(sensor));
		return false;
	}

	*cmd_channel = it_channel->second;

	return true;
}


bool csensor_event_listener::close_command_channel(sensor_type sensor)
{
	sensor_command_channel_map::iterator it_channel;

	it_channel = m_command_channels.find(sensor);

	if (it_channel == m_command_channels.end()) {
		ERR("%s doesn't have command_channel for %s", get_client_name(), get_sensor_name(sensor));
		return false;
	}

	delete it_channel->second;

	m_command_channels.erase(it_channel);

	return true;
}


bool csensor_event_listener::has_client_id(void)
{
	return (m_client_id != CLIENT_ID_INVALID);
}

int csensor_event_listener::get_client_id(void)
{
	return m_client_id;
}

void csensor_event_listener::set_client_id(const int client_id)
{
	m_client_id = client_id;
}

unsigned int csensor_event_listener::get_active_min_interval(const sensor_type sensor)
{
	unsigned int min_interval = POLL_MAX_HZ_MS;
	bool active_sensor_found = false;
	unsigned int interval;

	AUTOLOCK(m_handle_info_lock);

	sensor_handle_info_map::iterator it_handle;

	it_handle = m_sensor_handle_infos.begin();

	while (it_handle != m_sensor_handle_infos.end()) {
		if ((it_handle->second.m_sensor_type == sensor) &&
			(it_handle->second.m_sensor_state == SENSOR_STATE_STARTED)) {
				active_sensor_found = true;
				interval = it_handle->second.get_min_interval();
				min_interval = (interval < min_interval) ? interval : min_interval;
		}

		++it_handle;
	}

	if (!active_sensor_found)
		DBG("Active sensor[0x%x] is not found for client %s", sensor, get_client_name());

	return (active_sensor_found) ? min_interval : 0;

}

unsigned int csensor_event_listener::get_active_option(const sensor_type sensor)
{
	int active_option = SENSOR_OPTION_DEFAULT;
	bool active_sensor_found = false;
	int option;

	AUTOLOCK(m_handle_info_lock);

	sensor_handle_info_map::iterator it_handle;

	it_handle = m_sensor_handle_infos.begin();

	while (it_handle != m_sensor_handle_infos.end()) {
		if ((it_handle->second.m_sensor_type == sensor) &&
			(it_handle->second.m_sensor_state == SENSOR_STATE_STARTED)) {
				active_sensor_found = true;
				option = it_handle->second.m_sensor_option;
				active_option = (option > active_option) ? option : active_option;
		}

		++it_handle;
	}

	if (!active_sensor_found)
		DBG("Active sensor[0x%x] is not found for client %s", sensor, get_client_name());

	return active_option;
}

bool csensor_event_listener::get_sensor_type(const int handle, sensor_type &sensor)
{
	sensor_handle_info_map::iterator it_handle;

	AUTOLOCK(m_handle_info_lock);

	it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	sensor = it_handle->second.m_sensor_type;

	return true;
}

bool csensor_event_listener::get_sensor_state(const int handle, int &sensor_state)
{
	sensor_handle_info_map::iterator it_handle;

	AUTOLOCK(m_handle_info_lock);

	it_handle = m_sensor_handle_infos.find(handle);

	if (it_handle == m_sensor_handle_infos.end()) {
		ERR("Handle[%d] is not found for client %s", handle, get_client_name());
		return false;
	}

	sensor_state = it_handle->second.m_sensor_state;

	return true;
}

void csensor_event_listener::get_active_event_types(const sensor_type sensor,
		event_type_vector &active_event_types)
{
	event_type_vector event_types;

	AUTOLOCK(m_handle_info_lock);

	sensor_handle_info_map::iterator it_handle;

	it_handle = m_sensor_handle_infos.begin();
	while (it_handle != m_sensor_handle_infos.end()) {
		if ((it_handle->second.m_sensor_type == sensor) &&
			(it_handle->second.m_sensor_state == SENSOR_STATE_STARTED))
				it_handle->second.get_reg_event_types(event_types);

		++it_handle;
	}

	if (event_types.empty())
		return;

	sort(event_types.begin(), event_types.end());

	unique_copy(event_types.begin(), event_types.end(), back_inserter(active_event_types));

}


void csensor_event_listener::get_all_handles(handle_vector &handles)
{
	sensor_handle_info_map::iterator it_handle;

	AUTOLOCK(m_handle_info_lock);

	it_handle = m_sensor_handle_infos.begin();

	while (it_handle != m_sensor_handle_infos.end()) {
		handles.push_back(it_handle->first);
		++it_handle;
	}
}

bool csensor_event_listener::is_sensor_registered(const sensor_type sensor)
{
	sensor_handle_info_map::iterator it_handle;

	AUTOLOCK(m_handle_info_lock);

	it_handle = m_sensor_handle_infos.begin();

	while (it_handle != m_sensor_handle_infos.end()) {
		if (it_handle->second.m_sensor_type == sensor)
			return true;

		++it_handle;
	}

	return false;
}


bool csensor_event_listener::is_sensor_active(const sensor_type sensor)
{
	sensor_handle_info_map::iterator it_handle;

	AUTOLOCK(m_handle_info_lock);

	it_handle = m_sensor_handle_infos.begin();

	while (it_handle != m_sensor_handle_infos.end()) {
		if ((it_handle->second.m_sensor_type == sensor) &&
			(it_handle->second.m_sensor_state == SENSOR_STATE_STARTED))
			return true;

		++it_handle;
	}

	return false;
}

void csensor_event_listener::handle_events(void* event)
{
	const unsigned int MS_TO_US = 1000;
	const float MIN_DELIVERY_DIFF_FACTOR = 0.75f;

	unsigned long long cur_time;
	long long diff_time;
	creg_event_info event_info;
	sensor_event_data_t event_data;
	int situation;

	sensor_data_t sensor_data;
	sensorhub_data_t sensorhub_data;
	sensor_panning_data_t panning_data;
	int single_state_event_data = 0;

	int data_accuracy = ACCURACY_GOOD;

	unsigned int event_type = *((unsigned int *)(event));
	bool is_hub_event = is_sensorhub_event(event_type);

	client_callback_info* callback_info;
	vector<client_callback_info *> client_callback_infos;

	sensor_handle_info_map::iterator it_handle;

	if (is_hub_event) {
		sensorhub_event_t *sensor_hub_event = (sensorhub_event_t *)event;

		sensorhub_event_to_hub_data(*sensor_hub_event, sensorhub_data);
		event_data.event_data = &sensorhub_data;
		event_data.event_data_size = sizeof(sensorhub_data);

		situation = sensor_hub_event->situation;
		cur_time = sensor_hub_event->timestamp;
	} else {
		sensor_event_t *sensor_event = (sensor_event_t *)event;
		situation = sensor_event->situation;
		cur_time = sensor_event->timestamp;

		if (is_single_state_event(event_type)) {
			single_state_event_data = (int) sensor_event->values[0];
			event_data.event_data = (void *)&(single_state_event_data);
			event_data.event_data_size = sizeof(single_state_event_data);
		} else if (is_panning_event(event_type)) {
			panning_data.x = (int)sensor_event->values[0];
			panning_data.y = (int)sensor_event->values[1];
			event_data.event_data = (void *)&panning_data;
			event_data.event_data_size = sizeof(panning_data);
		} else {
			sensor_event_to_data(*sensor_event, sensor_data);
			event_data.event_data = (void *)&sensor_data;
			event_data.event_data_size = sizeof(sensor_data);

			data_accuracy = sensor_event->data_accuracy;
		}
	}

	{	/* scope for the lock */
		AUTOLOCK(m_handle_info_lock);

		for (it_handle = m_sensor_handle_infos.begin(); it_handle != m_sensor_handle_infos.end(); ++it_handle) {

			csensor_handle_info &sensor_handle_info = it_handle->second;

			if ((sensor_handle_info.m_sensor_state != SENSOR_STATE_STARTED) || !sensor_handle_info.get_reg_event_info(event_type, event_info))
				continue;

			if ((sensor_handle_info.m_sensor_option != SENSOR_OPTION_ALWAYS_ON) &&
					((situation == SITUATION_LCD_OFF) || (situation == SITUATION_SURVIVAL_MODE)))
				continue;

			if (event_info.m_fired)
				continue;

			diff_time = cur_time - event_info.m_previous_event_time;

			if ((diff_time >= event_info.m_event_interval * MS_TO_US * MIN_DELIVERY_DIFF_FACTOR) || ((diff_time > 0) && !is_ontime_event(event_type))) {
				unsigned int cal_event_type;
				creg_event_info cal_event_info;

				event_info.m_previous_event_time = cur_time;

				cal_event_type = get_calibration_event_type(event_type);

				if (cal_event_type) {
					if ((data_accuracy == ACCURACY_BAD) && !sensor_handle_info.bad_accuracy &&
						sensor_handle_info.get_reg_event_info(cal_event_type, cal_event_info)) {
						sensor_event_data_t cal_event_data;
						client_callback_info* cal_callback_info;

						cal_event_info.m_previous_event_time = cur_time;
						cal_event_data.event_data = (void *)&(data_accuracy);
						cal_event_data.event_data_size = sizeof(data_accuracy);
						cal_callback_info = get_callback_info(cal_event_info, cal_event_data);

						client_callback_infos.push_back(cal_callback_info);
						sensor_handle_info.bad_accuracy = true;

						print_event_occurrence_log(sensor_handle_info, cal_event_info, cal_event_data);
					}

					if ((data_accuracy != ACCURACY_BAD) && sensor_handle_info.bad_accuracy)
						sensor_handle_info.bad_accuracy = false;
				}

				callback_info = get_callback_info(event_info, event_data);
				client_callback_infos.push_back(callback_info);

				if (is_one_shot_event(event_type))
					event_info.m_fired = true;

				print_event_occurrence_log(sensor_handle_info, event_info, event_data);
			}
		}
	}

	vector<client_callback_info *>::iterator it_calback_info;

	it_calback_info = client_callback_infos.begin();

	while (it_calback_info != client_callback_infos.end()) {
		post_callback_to_main_loop(*it_calback_info);
		++it_calback_info;
	}

}


client_callback_info* csensor_event_listener::get_callback_info(creg_event_info &event_info, sensor_event_data_t &event_data)
{
	client_callback_info* callback_info;

	callback_info = new client_callback_info;

	callback_info->callback = event_info.m_event_callback;
	callback_info->event_type = event_info.m_event_type;

	callback_info->event_data.event_data_size = event_data.event_data_size;
	callback_info->event_data.event_data = new char[event_data.event_data_size];
	memcpy(callback_info->event_data.event_data, event_data.event_data, event_data.event_data_size);

	callback_info->data = event_info.m_cb_data;

	return callback_info;
}

void csensor_event_listener::post_callback_to_main_loop(client_callback_info* cb_info)
{
	g_idle_add_full(G_PRIORITY_DEFAULT, callback_dispatcher, cb_info, NULL);

}

gboolean csensor_event_listener::callback_dispatcher(gpointer data)
{
	client_callback_info *cb_info =  (client_callback_info*) data;

	cb_info->callback(cb_info->event_type, &cb_info->event_data, cb_info->data);

	delete[] (char*)(cb_info->event_data.event_data);
	delete cb_info;

/*
* 	To be called only once, it returns false
*/
	return false;
}



bool csensor_event_listener::sensor_event_poll(void* buffer, int buffer_len)
{
	ssize_t len;

	len = m_event_socket.recv(buffer, buffer_len);

	if (!len) {
		if(!m_poller->poll())
			return false;
		len = m_event_socket.recv(buffer, buffer_len);

		if (!len) {
			INFO("%s failed to read after poll!", get_client_name());
			return false;
		}
	}

	if (len < 0) {
		INFO("%s failed to recv event from event socket", get_client_name());
		return false;
	}

	return true;
}



void csensor_event_listener::listen_events(void)
{
	sensorhub_event_t buffer;

	do {
		lock l(m_thread_mutex);
		if (m_thread_state == THREAD_STATE_START) {
			if (!sensor_event_poll(&buffer, sizeof(buffer))) {
				INFO("sensor_event_poll failed");
				break;
			}

			handle_events(&buffer);
		} else {
			break;
		}
	} while (true);

	if (m_poller != NULL) {
		delete m_poller;
		m_poller = NULL;
	}

	close_event_channel();
	set_client_id(CLIENT_ID_INVALID);

	lock l(m_thread_mutex);
	m_thread_state = THREAD_STATE_TERMINATE;
	m_thread_cond.notify_one();

	INFO("Event listener thread is teminated.");
}

bool csensor_event_listener::create_event_channel(void)
{
	int client_id;
	event_channel_ready_t event_channel_ready;

	if (!m_event_socket.create())
		return false;

	if (!m_event_socket.connect(EVENT_CHANNEL_PATH)) {
		ERR("Failed to connect event channel for client %s, event socket fd[%d]", get_client_name(), m_event_socket.get_socket_fd());
		return false;
	}

	m_event_socket.set_connection_mode();

	client_id = get_client_id();

	if (m_event_socket.send(&client_id, sizeof(client_id)) <= 0) {
		ERR("Failed to send client id for client %s on event socket[%d]", get_client_name(), m_event_socket.get_socket_fd());
		return false;
	}

	if (m_event_socket.recv(&event_channel_ready, sizeof(event_channel_ready)) <= 0) {
		ERR("%s failed to recv event_channel_ready packet on event socket[%d] with client id [%d]",
			get_client_name(), m_event_socket.get_socket_fd(), client_id);
		return false;
	}

	if ((event_channel_ready.magic != EVENT_CHANNEL_MAGIC) || (event_channel_ready.client_id != client_id)) {
		ERR("Event_channel_ready packet is wrong, magic = 0x%x, client id = %d",
			event_channel_ready.magic, event_channel_ready.client_id);
		return false;
	}

	INFO("Event channel is established for client %s on socket[%d] with client id : %d",
		get_client_name(), m_event_socket.get_socket_fd(), client_id);

	return true;
}


void csensor_event_listener::close_event_channel(void)
{
	m_event_socket.close();
}


void csensor_event_listener::stop_event_listener(void)
{
	const int THREAD_TERMINATING_TIMEOUT = 2;

	ulock u(m_thread_mutex);

	if (m_thread_state != THREAD_STATE_TERMINATE) {
		m_thread_state = THREAD_STATE_STOP;

		_D("%s is waiting listener thread[state: %d] to be teminated", get_client_name(), m_thread_state);
		if (m_thread_cond.wait_for(u, std::chrono::seconds(THREAD_TERMINATING_TIMEOUT))
			== std::cv_status::timeout)
			_E("Fail to stop listener thread after waiting %d seconds", THREAD_TERMINATING_TIMEOUT);
		else
			_D("Listener thread for %s is teminated", get_client_name());
	}
}

void csensor_event_listener::set_thread_state(thread_state state)
{
	lock l(m_thread_mutex);
	m_thread_state = state;
}

bool csensor_event_listener::start_event_listener(void)
{
	if (!create_event_channel()) {
		ERR("Event channel is not established for %s", get_client_name());
		return false;
	}

	m_event_socket.set_transfer_mode();

	m_poller = new poller(m_event_socket.get_socket_fd());

	set_thread_state(THREAD_STATE_START);

	thread listener(&csensor_event_listener::listen_events, this);
	listener.detach();

	return true;
}


