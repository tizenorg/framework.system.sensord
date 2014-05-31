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

#include <cclient_sensor_record.h>
#include <common.h>

using std::pair;

static sensor_type get_sensor_type(const unsigned int event_type)
{
	const int SENSOR_TYPE_SHIFT = 16;
	return (sensor_type) (event_type >> SENSOR_TYPE_SHIFT);
}

cclient_sensor_record::cclient_sensor_record()
: m_client_id(0)
, m_pid(-1)
{

}

cclient_sensor_record::~cclient_sensor_record()
{
	m_sensor_usages.clear();
	close_event_socket();
}

bool cclient_sensor_record::register_event(const unsigned int event_type)
{
	sensor_usage_map::iterator it_usage;

	sensor_type sensor = get_sensor_type(event_type);

	it_usage = m_sensor_usages.find(sensor);

	if (it_usage == m_sensor_usages.end()) {
		csensor_usage usage;
		usage.register_event(event_type);
		m_sensor_usages.insert(pair<sensor_type,csensor_usage>(sensor, usage));
		return true;
	}

	if (!it_usage->second.register_event(event_type)) {
		ERR("Event[0x%x] is already registered", event_type);
		return false;
	}

	return true;
}

bool cclient_sensor_record::unregister_event(const unsigned int event_type)
{
	sensor_usage_map::iterator it_usage;

	sensor_type sensor = get_sensor_type(event_type);

	it_usage = m_sensor_usages.find(sensor);

	if (it_usage == m_sensor_usages.end()) {
		ERR("Sensor[0x%x] is not registered", sensor);
		return false;
	}

	if (!it_usage->second.unregister_event(event_type)) {
		ERR("Event[0x%x] is already registered", event_type);
		return false;
	}

	return true;
}

bool cclient_sensor_record::set_interval(const sensor_type sensor, const unsigned int interval)
{
	sensor_usage_map::iterator it_usage;

	it_usage = m_sensor_usages.find(sensor);

	if (it_usage == m_sensor_usages.end()) {
		csensor_usage usage;
		usage.m_interval = interval;
		m_sensor_usages.insert(pair<sensor_type, csensor_usage>(sensor, usage));
	} else {
		it_usage->second.m_interval = interval;
	}

	return true;
}

bool cclient_sensor_record::set_option(const sensor_type sensor, const int option)
{
	sensor_usage_map::iterator it_usage;

	it_usage = m_sensor_usages.find(sensor);

	if (it_usage == m_sensor_usages.end()) {
		csensor_usage usage;
		usage.m_option = option;
		m_sensor_usages.insert(pair<sensor_type, csensor_usage>(sensor, usage));
	} else {
		it_usage->second.m_option = option;
	}

	return true;
}

unsigned int cclient_sensor_record::get_interval(const sensor_type sensor)
{
	sensor_usage_map::iterator it_usage;

	it_usage = m_sensor_usages.find(sensor);

	if (it_usage == m_sensor_usages.end()) {
		ERR("Sensor[0x%x] is not found", sensor);
		return 0;
	}

	return it_usage->second.m_interval;
}

bool cclient_sensor_record::is_sensor_used(const sensor_type sensor, const event_situation mode)
{
	sensor_usage_map::iterator it_usage;

	it_usage = m_sensor_usages.find(sensor);

	if (it_usage == m_sensor_usages.end())
		return false;

	if ((mode == SITUATION_LCD_OFF || mode == SITUATION_SURVIVAL_MODE) &&
		(it_usage->second.m_option != SENSOR_OPTION_ALWAYS_ON))
		return false;

	return true;
}

bool cclient_sensor_record::is_listening_event(const unsigned int event_type, const event_situation mode)
{

	sensor_usage_map::iterator it_usage;

	sensor_type sensor = get_sensor_type(event_type);

	it_usage = m_sensor_usages.find(sensor);

	if (it_usage == m_sensor_usages.end())
		return false;

	if ((mode == SITUATION_LCD_OFF || mode == SITUATION_SURVIVAL_MODE) &&
		(it_usage->second.m_option != SENSOR_OPTION_ALWAYS_ON))
		return false;

	if (it_usage->second.is_event_registered(event_type))
		return true;

	return false;
}

bool cclient_sensor_record::add_sensor_usage(const sensor_type sensor)
{
	sensor_usage_map::iterator it_usage;

	it_usage = m_sensor_usages.find(sensor);

	if (it_usage != m_sensor_usages.end()) {
		ERR("Sensor[0x%x] is already registered", sensor);
		return false;
	}

	csensor_usage usage;
	m_sensor_usages.insert(pair<sensor_type,csensor_usage> (sensor, usage));
	return true;
}

bool cclient_sensor_record::remove_sensor_usage(const sensor_type sensor)
{
	sensor_usage_map::iterator it_usage;

	it_usage = m_sensor_usages.find(sensor);

	if (it_usage == m_sensor_usages.end()) {
		ERR("Sensor[0x%x] is not found", sensor);
		return false;
	}
	m_sensor_usages.erase(it_usage);
	return true;
}

bool cclient_sensor_record::has_sensor_usage(void)
{
	if (m_sensor_usages.empty())
		return false;

	return true;
}


bool cclient_sensor_record::has_sensor_usage(const sensor_type sensor)
{
	sensor_usage_map::iterator it_usage;

	it_usage = m_sensor_usages.find(sensor);

	if (it_usage == m_sensor_usages.end()) {
		DBG("Sensor[0x%x] is not found", sensor);
		return false;
	}

	return true;
}


bool cclient_sensor_record::get_registered_events(const sensor_type sensor, event_type_vector &event_vec)
{
	sensor_usage_map::iterator it_usage;

	it_usage = m_sensor_usages.find(sensor);

	if (it_usage == m_sensor_usages.end()) {
		DBG("Sensor[0x%x] is not found", sensor);
		return false;
	}

	copy(it_usage->second.m_reg_events.begin(), it_usage->second.m_reg_events.end(), back_inserter(event_vec));

	return true;

}

void cclient_sensor_record::set_client_id(int client_id)
{
	m_client_id = client_id;
}

void cclient_sensor_record::set_client_info(pid_t pid)
{
	char client_info[NAME_MAX + 32];
	char proc_name[NAME_MAX];

	m_pid = pid;
	get_proc_name(pid, proc_name);

	snprintf(client_info, sizeof(client_info), "%s[pid=%d, id=%d]", proc_name, m_pid, m_client_id);
	m_client_info.assign(client_info);

}

const char* cclient_sensor_record::get_client_info(void)
{
	return m_client_info.c_str();
}


void cclient_sensor_record::set_event_socket(const cevent_socket socket)
{
	m_event_socket = socket;
}

void cclient_sensor_record::get_event_socket(cevent_socket &socket)
{
	socket = m_event_socket;
}


bool cclient_sensor_record::close_event_socket(void)
{
	return m_event_socket.close();
}


