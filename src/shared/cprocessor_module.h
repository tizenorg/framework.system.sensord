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

#if !defined(_CPROCESSOR_CLASS_H_)
#define _CPROCESSOR_CLASS_H_
#include <list>
#include <map>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <cinterval_info_list.h>
using std::mutex;
using std::recursive_mutex;
using std::lock_guard;
using std::list;
using std::map;
using std::vector;
using std::unique_lock;
using std::condition_variable;


class cprocessor_module : public cmodule
{
private:
	typedef map<unsigned int,unsigned int> client_info;

public:
	static const int SF_PLUGIN_PROCESSOR = SF_PLUGIN_BASE + 20;

	cprocessor_module();
	virtual ~cprocessor_module();

	virtual int get_processor_type(void) = 0;

	virtual bool add_input(csensor_module *sensor) = 0;
	virtual bool add_input(cprocessor_module *processor) = 0;

	virtual cprocessor_module *create_new(void) = 0;
	virtual void destroy(cprocessor_module *module) = 0;

	virtual bool start(void);
	virtual bool stop(void);

	virtual void set_main(void *(*main)(void *), void *(stopped)(void *), void *arg);

	virtual long set_cmd(int type , int property , long input_value) = 0;
	virtual int get_property(unsigned int property_level , void *property_struct ) = 0;
	virtual int get_struct_value(unsigned int struct_type , void *struct_values) = 0;

	void register_supported_event(unsigned int event_type);
	bool is_supported(unsigned int event_type);
	virtual bool add_client(unsigned int event_type);
	virtual bool delete_client(unsigned int event_type);
	bool add_interval(int client_id, unsigned int interval, bool is_processor = false);
	bool delete_interval(int client_id, bool is_processor = false);
	unsigned int get_interval(int client_id, bool is_processor = false);

	virtual int send_sensorhub_data(const char* data, int data_len);
protected:
	typedef lock_guard<mutex> lock;
	typedef lock_guard<recursive_mutex> rlock;
	typedef unique_lock<mutex> ulock;

	cworker *m_worker;

	cinterval_info_list m_interval_info_list;
	cmutex m_interval_info_list_mutex;

	client_info m_client_info;
	cmutex m_client_info_mutex;

	bool push(sensor_event_t const &event);
	bool push(sensorhub_event_t const &event);
	unsigned int get_client_cnt(unsigned int event_type);
	virtual bool update_polling_interval(unsigned long val) = 0;
	virtual void base_data_to_sensor_event(base_data_struct *base, sensor_event_t *event);

private:
	vector<int> m_supported_event_info;
};

#endif

//! End of a file
