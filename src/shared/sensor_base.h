/*
 * libsensord-share
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
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

#ifndef _SENSOR_BASE_H_
#define _SENSOR_BASE_H_

#include <list>
#include <map>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <string>

#include <cinterval_info_list.h>
#include <cmutex.h>

#include <common.h>
#include <sensor_common.h>
#include <worker_thread.h>
#include <sensor_info.h>

class sensor_base
{
private:
	typedef std::unordered_map<unsigned int, unsigned int> client_info;

public:
	sensor_base();
	virtual ~sensor_base();

	virtual bool init(void);
	void add_id(sensor_id_t id);
	sensor_id_t get_id(sensor_type_t sensor_type);
	virtual void get_types(std::vector<sensor_type_t> &types) {};

	sensor_privilege_t get_privilege(void);
	int get_permission(void);
	virtual const char* get_name(void);
	virtual bool is_virtual(void);
	virtual bool is_fusion(void);

	bool start(void);
	bool stop(void);
	bool is_started(void);

	virtual bool add_client(unsigned int event_type);
	virtual bool delete_client(unsigned int event_type);

	virtual bool add_interval(int client_id, unsigned int interval, bool is_processor);
	virtual bool delete_interval(int client_id, bool is_processor);
	unsigned int get_interval(int client_id, bool is_processor);

	void get_sensor_info(sensor_type_t sensor_type, sensor_info &info);
	virtual bool get_properties(sensor_type_t sensor_type, sensor_properties_t &properties);

	bool is_supported(unsigned int event_type);

	virtual long set_command(unsigned int cmd, long value);
	virtual int send_sensorhub_data(const char* data, int data_len);

	virtual int get_sensor_data(unsigned int type, sensor_data_t &data);

protected:
	typedef std::lock_guard<std::mutex> lock;
	typedef std::lock_guard<std::recursive_mutex> rlock;
	typedef std::unique_lock<std::mutex> ulock;

	std::map<sensor_type_t, sensor_id_t> m_ids;
	sensor_privilege_t m_privilege;
	int m_permission;

	cinterval_info_list m_interval_info_list;
	cmutex m_interval_info_list_mutex;

	cmutex m_mutex;

	unsigned int m_client;
	cmutex m_client_mutex;

	client_info m_client_info;
	cmutex m_client_info_mutex;

	std::vector<unsigned int> m_supported_event_info;
	bool m_started;

	std::string m_name;

	sensor_id_t get_id(void);
	void set_privilege(sensor_privilege_t privilege);
	void set_permission(int permission);
	unsigned int get_client_cnt(unsigned int event_type);
	virtual bool set_interval(unsigned long val);
	void register_supported_event(unsigned int event_type);

	static unsigned long long get_timestamp(void);
	static unsigned long long get_timestamp(timeval *t);
private:
	virtual bool on_start(void);
	virtual bool on_stop(void);
};

#endif
