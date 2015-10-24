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

#ifndef _BATCH_INFO_LIST_CLASS_H_
#define _BATCH_INFO_LIST_CLASS_H_

#include <unordered_map>
#include <memory>

class batch_info
{
public:
	batch_info(unsigned int interval, unsigned int latency);
	unsigned int interval;
	unsigned int latency;
};

class batch_info_list
{
private:
	std::unordered_map<int, std::shared_ptr<batch_info>> m_batch_infos;

public:
	bool add_batch(int id, unsigned int interval, unsigned int latency);
	bool delete_batch(int id);
	bool get_batch(int id, unsigned int &interval, unsigned int &latency);
	bool get_best_batch(unsigned int &interval, unsigned int &latency);
};
#endif
