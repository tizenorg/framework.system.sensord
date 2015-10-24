/*
 * libsensord-share
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
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

#include <batch_info_list.h>
#include <algorithm>

using std::pair;
using std::shared_ptr;

batch_info::batch_info(unsigned int interval, unsigned int latency)
{
	this->interval = interval;
	this->latency = latency;
}

bool batch_info_list::add_batch(int id, unsigned int interval, unsigned int latency)
{
	auto it_batch_info = m_batch_infos.find(id);

	if (it_batch_info != m_batch_infos.end()) {
		it_batch_info->second->interval = interval;
		it_batch_info->second->latency = latency;
		return true;
	}

	m_batch_infos.insert(pair<int, shared_ptr<batch_info>> (id, std::make_shared<batch_info> (interval, latency)));
	return true;
}

bool batch_info_list::delete_batch(int id)
{
	auto it_batch_info = m_batch_infos.find(id);

	if (it_batch_info == m_batch_infos.end())
		return false;

	m_batch_infos.erase(it_batch_info);
	return true;
}

bool batch_info_list::get_batch(int id, unsigned int &interval, unsigned int &latency)
{
	auto it_batch_info = m_batch_infos.find(id);

	if (it_batch_info == m_batch_infos.end())
		return false;

	interval = it_batch_info->second->interval;
	latency = it_batch_info->second->latency;

	return true;
}

bool batch_info_list::get_best_batch(unsigned int &interval, unsigned int &latency)
{
	if (m_batch_infos.empty())
		return false;

	auto get_min_interval = [](pair<const int, shared_ptr<batch_info>> &a, pair<const int, shared_ptr<batch_info>> &b){
		return (a.second->interval < b.second->interval);
	};

	auto get_min_latency = [](pair<const int, shared_ptr<batch_info>> &a, pair<const int, shared_ptr<batch_info>> &b){
		return (a.second->latency < b.second->latency);
	};

	auto it_interval_min = std::min_element(m_batch_infos.begin(), m_batch_infos.end(), get_min_interval);
	auto it_latency_min = std::min_element(m_batch_infos.begin(), m_batch_infos.end(), get_min_latency);

	interval = it_interval_min->second->interval;
	latency = it_latency_min->second->latency;

	return true;
}
