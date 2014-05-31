/*
 * sensor-framework
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

#include <cclient_info_manager.h>
#include <common.h>
#include <cevent_socket.h>

using std::pair;

cclient_info_manager cclient_info_manager::inst;


cclient_info_manager::cclient_info_manager()
{
}
cclient_info_manager::~cclient_info_manager()
{
	m_clients.clear();
}

unsigned int cclient_info_manager::get_interval(const int client_id, const sensor_type sensor)
{
	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;

	it_record = m_clients.find(client_id);

	if (it_record == m_clients.end()) {
		ERR("Client[%d] is not found", client_id);
		return 0;
	}

	return it_record->second.get_interval(sensor);
}

bool cclient_info_manager::is_sensor_used(const sensor_type sensor, const event_situation mode)
{
	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;

	it_record = m_clients.begin();

	while (it_record != m_clients.end()) {
		if(it_record->second.is_sensor_used(sensor, mode))
			return true;

		++it_record;
	}

	return false;
}


bool cclient_info_manager::get_registered_events(const int client_id, const sensor_type sensor, event_type_vector &event_vec)
{
	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;

	it_record = m_clients.find(client_id);

	if (it_record == m_clients.end()) {
		ERR("Client[%d] is not found", client_id);
		return false;
	}

	if(!it_record->second.get_registered_events(sensor, event_vec))
		return false;

	return true;
}


bool cclient_info_manager::register_event(const int client_id, const unsigned int event_type)
{
	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;

	it_record = m_clients.find(client_id);

	if (it_record == m_clients.end()) {
		ERR("Client[%d] is not found", client_id);
		return false;
	}

	if(!it_record->second.register_event(event_type))
		return false;

	return true;
}

bool cclient_info_manager::unregister_event(const int client_id, const unsigned int event_type)
{
	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;

	it_record = m_clients.find(client_id);

	if (it_record == m_clients.end()) {
		ERR("Client[%d] is not found", client_id);
		return false;
	}

	if(!it_record->second.unregister_event(event_type))
		return false;

	return true;
}

bool cclient_info_manager::set_interval(const int client_id, const sensor_type sensor, const unsigned int interval)
{
	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;
	it_record = m_clients.find(client_id);

	if (it_record == m_clients.end()) {
		ERR("Client[%d] is not found", client_id);
		return false;
	}

	if(!it_record->second.set_interval(sensor, interval))
		return false;

	return true;
}

bool cclient_info_manager::set_option(const int client_id, const sensor_type sensor, const int option)
{
	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;
	it_record = m_clients.find(client_id);

	if (it_record == m_clients.end()) {
		ERR("Client[%d] is not found", client_id);
		return false;
	}

	if(!it_record->second.set_option(sensor, option))
		return false;

	return true;
}


int cclient_info_manager::create_client_record(void)
{
	AUTOLOCK(m_mutex);

	int client_id = 0;

	cclient_sensor_record client_record;

	while (m_clients.count(client_id) > 0)
		client_id++;

	if (client_id == MAX_HANDLE) {
		ERR("Sensor records of clients are full");
		return MAX_HANDLE_REACHED;
	}

	client_record.set_client_id(client_id);

	m_clients.insert(pair<int,cclient_sensor_record> (client_id, client_record));

	return client_id;
}


bool cclient_info_manager::remove_client_record(const int client_id)
{
	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;

	it_record = m_clients.find(client_id);

	if (it_record == m_clients.end()) {
		ERR("Client[%d] is not found", client_id);
		return false;
	}

	m_clients.erase(it_record);

	INFO("Client record for client[%d] is removed from client info manager", client_id);

	return true;
}


bool cclient_info_manager::has_client_record(int client_id)
{
	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;

	it_record = m_clients.find(client_id);

	return (it_record != m_clients.end());
}


void cclient_info_manager::set_client_info(int client_id, pid_t pid)
{
	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;

	it_record = m_clients.find(client_id);

	if (it_record == m_clients.end()) {
		ERR("Client[%d] is not found", client_id);
		return;
	}

	it_record->second.set_client_info(pid);

	return;
}

const char* cclient_info_manager::get_client_info(int client_id)
{
	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;

	it_record = m_clients.find(client_id);

	if (it_record == m_clients.end()) {
		ERR("Client[%d] is not found", client_id);
		return NULL;
	}

	return it_record->second.get_client_info();
}

bool cclient_info_manager::create_sensor_record(int client_id, const sensor_type sensor)
{
	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;

	it_record = m_clients.find(client_id);

	if (it_record == m_clients.end()) {
		ERR("Client record[%d] is not registered", client_id);
		return false;
	}

	it_record->second.add_sensor_usage(sensor);

	return true;
}

bool cclient_info_manager::remove_sensor_record(const int client_id, const sensor_type sensor)
{
	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;

	it_record = m_clients.find(client_id);

	if (it_record == m_clients.end()) {
		ERR("Client[%d] is not found", client_id);
		return false;
	}

	if(!it_record->second.remove_sensor_usage(sensor))
		return false;

	if(!it_record->second.has_sensor_usage())
		remove_client_record(client_id);

	return true;
}


bool cclient_info_manager::has_sensor_record(const int client_id, const sensor_type sensor)
{
	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;

	it_record = m_clients.find(client_id);

	if (it_record == m_clients.end()) {
		DBG("Client[%d] is not found", client_id);
		return false;
	}

	if(!it_record->second.has_sensor_usage(sensor))
		return false;

	return true;
}

bool cclient_info_manager::has_sensor_record(const int client_id)
{
	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;

	it_record = m_clients.find(client_id);

	if (it_record == m_clients.end()) {
		DBG("Client[%d] is not found", client_id);
		return false;
	}

	if(!it_record->second.has_sensor_usage())
		return false;

	return true;
}

bool cclient_info_manager::get_listener_ids(const unsigned int event_type, const event_situation mode, client_id_vec &id_vec)
{
	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;

	it_record = m_clients.begin();	
	while (it_record != m_clients.end()) {
		if(it_record->second.is_listening_event(event_type, mode))
			id_vec.push_back(it_record->first);

		++it_record;
	}

	return true;
}

bool cclient_info_manager::get_event_socket(const int client_id, cevent_socket &socket)
{
	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;

	it_record = m_clients.find(client_id);

	if (it_record == m_clients.end()) {
		ERR("Client[%d] is not found", client_id);
		return false;
	}

	it_record->second.get_event_socket(socket);

	return true;
}

bool cclient_info_manager::set_event_socket(const int client_id, const cevent_socket socket)
{

	AUTOLOCK(m_mutex);

	client_id_sensor_record_map::iterator it_record;

	it_record = m_clients.find(client_id);

	if (it_record == m_clients.end()) {
		ERR("Client[%d] is not found", client_id);
		return false;
	}

	it_record->second.set_event_socket(socket);

	return true;
}
