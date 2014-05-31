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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dlfcn.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/time.h>
#include <cobject_type.h>
#include <cmutex.h>
#include <clist.h>
#include <cmodule.h>
#include "common.h"
#include <cpacket.h>
#include <cworker.h>
#include <cipc_worker.h>
#include <csock.h>
#include <sf_common.h>
#include <csensor_module.h>
#include <cfilter_module.h>
#include <cprocessor_module.h>
#include <csensor_event_queue.h>
#include <algorithm>

#define BASE_GATHERING_INTERVAL 1000
#define MS_TO_US 1000

cprocessor_module::cprocessor_module()
: m_worker(NULL)
{
	ctype::set_type(SF_PLUGIN_PROCESSOR);

	try {
		m_worker = new cworker();
	} catch (...) {
		throw ENOMEM;
	}
}

cprocessor_module::~cprocessor_module()
{
	if(m_worker) {
		m_worker->terminate();
	}
}

void cprocessor_module::set_main(void *(*main)(void *), void*(*stopped)(void*), void *arg)
{
	m_worker->set_stopped(stopped);
	m_worker->set_started(main);
	m_worker->set_context(arg);
}

bool cprocessor_module::start(void)
{
	DBG("START#########################################\n");
	return m_worker->start();
}

bool cprocessor_module::stop(void)
{
	DBG("STOP#########################################\n");
	return m_worker->stop();
}

bool cprocessor_module::push(sensor_event_t const &event)
{
	csensor_event_queue::get_instance().push(event);
	return true;
}

bool cprocessor_module::push(sensorhub_event_t const &event)
{
	csensor_event_queue::get_instance().push(event);
	return true;
}

bool cprocessor_module::is_supported(unsigned int event_type)
{
	vector<int>::iterator iter;

	iter = find(m_supported_event_info.begin(), m_supported_event_info.end(), event_type);

	if (iter == m_supported_event_info.end())
		return false;

	return true;
}

void cprocessor_module::register_supported_event(unsigned int event_type)
{
	m_supported_event_info.push_back(event_type);
}

bool cprocessor_module::add_client(unsigned int event_type)
{
	AUTOLOCK(m_client_info_mutex);
	++(m_client_info[event_type]);
	return true;
}

bool cprocessor_module::delete_client(unsigned int event_type)
{
	AUTOLOCK(m_client_info_mutex);
	client_info::iterator iter;

	iter = m_client_info.find(event_type);

	if (iter == m_client_info.end())
		return false;

	if (iter->second == 0)
		return false;

	--(iter->second);

	return true;
}

unsigned int cprocessor_module::get_client_cnt(unsigned int event_type)
{
	AUTOLOCK(m_client_info_mutex);
	client_info::iterator iter;

	iter = m_client_info.find(event_type);

	if (iter == m_client_info.end())
		return 0;

	return iter->second;
}

void cprocessor_module::base_data_to_sensor_event(base_data_struct *base, sensor_event_t *event)
{
	event->timestamp = base->time_stamp;
	event->data_accuracy = base->data_accuracy;
	event->data_unit_idx = base->data_unit_idx;
	event->values_num = base->values_num;
	memcpy(event->values, base->values, sizeof(base->values));

}

bool cprocessor_module::add_interval(int client_id, unsigned int interval, bool is_processor)
{
	unsigned int cur_min;

	AUTOLOCK(m_interval_info_list_mutex);

	cur_min = m_interval_info_list.get_min();

	if (!m_interval_info_list.add_interval(client_id, interval, is_processor))
		return false;

	if ((interval < cur_min) || !cur_min) {
		INFO("Min interval for sensor[0x%x] is changed from %dms to %dms"
			" by%sclient[%d] adding interval",
			get_processor_type(), cur_min, interval,
			is_processor ? " processor " : " ", client_id);
		update_polling_interval(interval);
	}

	return true;
}

bool cprocessor_module::delete_interval(int client_id, bool is_processor)
{
	unsigned int prev_min, cur_min;
	AUTOLOCK(m_interval_info_list_mutex);

	prev_min = m_interval_info_list.get_min();

	if (!m_interval_info_list.delete_interval(client_id, is_processor))
		return false;

	cur_min = m_interval_info_list.get_min();

	if (!cur_min) {
		INFO("No interval for sensor[0x%x] by%sclient[%d] deleting interval, "
			 "so set to default %dms",
			 get_processor_type(), is_processor ? " processor " : " ",
			 client_id, POLL_1HZ_MS);

		update_polling_interval(POLL_1HZ_MS);
	} else if (cur_min != prev_min) {
		INFO("Min interval for sensor[0x%x] is changed from %dms to %dms"
			" by%sclient[%d] deleting interval",
			get_processor_type(), prev_min, cur_min,
			is_processor ? " processor " : " ", client_id);

		update_polling_interval(cur_min);
	}

	return true;
}

unsigned int cprocessor_module::get_interval(int client_id, bool is_processor)
{
	AUTOLOCK(m_interval_info_list_mutex);

	return m_interval_info_list.get_interval(client_id, is_processor);
}


int cprocessor_module::send_sensorhub_data(const char* data, int data_len)
{
	return -1;
}
