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

#include <client_common.h>
#include <csensor_handle_info.h>

using std::pair;

csensor_handle_info::csensor_handle_info()
: m_handle(0)
, m_sensor_type(UNKNOWN_SENSOR)
, m_sensor_state(SENSOR_STATE_UNKNOWN)
, m_sensor_option(SENSOR_OPTION_DEFAULT)
, bad_accuracy(false)
{

}

csensor_handle_info::~csensor_handle_info()
{
	clear_all_events();
}

bool csensor_handle_info::get_reg_event_info(unsigned int event_type, creg_event_info &event_info)
{
	event_info_map::iterator it_event;

	it_event = m_reg_event_infos.find(event_type);

	if (it_event == m_reg_event_infos.end()) {
		DBG("Event %s[0x%x] is not registered for client %s", get_event_name(event_type), event_type, get_client_name());
		return false;
	}

	event_info = it_event->second;
	return true;
}

void csensor_handle_info::get_reg_event_types(event_type_vector &event_types)
{
	event_info_map::iterator it_event;

	it_event = m_reg_event_infos.begin();

	while (it_event != m_reg_event_infos.end()) {
		event_types.push_back(it_event->first);
		++it_event;
	}
}

bool csensor_handle_info::add_reg_event_info(const unsigned int event_type, const unsigned int interval,
		const sensor_callback_func_t callback, void* cb_data)
{
	event_info_map::iterator it_event;
	creg_event_info event_info;

	it_event = m_reg_event_infos.find(event_type);

	if (it_event != m_reg_event_infos.end()) {
		ERR("Event %s[0x%x] is already registered for client %s", get_event_name(event_type), event_type, get_client_name());
		return false;
	}

	event_info.m_event_type = event_type;
	event_info.m_event_interval = interval;
	event_info.m_event_callback = callback;
	event_info.m_cb_data = cb_data;

	m_reg_event_infos.insert(pair<unsigned int,creg_event_info> (event_type,event_info));

	return true;
}

bool csensor_handle_info::delete_reg_event_info(const unsigned int event_type)
{
	event_info_map::iterator it_event;

	it_event = m_reg_event_infos.find(event_type);

	if (it_event == m_reg_event_infos.end()) {
		ERR("Event %s[0x%x] is not registered for client %s", get_event_name(event_type), event_type, get_client_name());
		return false;
	}

	m_reg_event_infos.erase(it_event);

	return true;
}

void csensor_handle_info::clear_all_events(void)
{
	m_reg_event_infos.clear();
}

bool csensor_handle_info::change_reg_event_interval(const unsigned int event_type, const unsigned int interval)
{
	event_info_map::iterator it_event;

	it_event = m_reg_event_infos.find(event_type);

	if (it_event == m_reg_event_infos.end()) {
		ERR("Event %s[0x%x] is not registered for client %s", get_event_name(event_type), event_type, get_client_name());
		return false;
	}

	it_event->second.m_event_interval = interval;

	return true;
}

unsigned int csensor_handle_info::get_min_interval(void)
{
	unsigned int min_interval = POLL_MAX_HZ_MS;
	unsigned int interval;

	if (m_reg_event_infos.empty()) {
		DBG("No events are registered for client %s", get_client_name());
		return min_interval;
	}

	event_info_map::iterator it_event;

	it_event = m_reg_event_infos.begin();

	while (it_event != m_reg_event_infos.end()) {
		interval = it_event->second.m_event_interval;
		min_interval = (interval < min_interval) ? interval : min_interval;
		++it_event;
	}

	return min_interval;
}

unsigned int csensor_handle_info::get_reg_event_count(void)
{
	return m_reg_event_infos.size();
}

