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

#include <csensor_event_queue.h>
#include "common.h">

csensor_event_queue csensor_event_queue::inst;

csensor_event_queue::csensor_event_queue()
{
}

void csensor_event_queue::push(sensor_event_t const &event)
{
	sensor_event_t *new_event = new sensor_event_t;
	*new_event = event;

	sensor_event_queue_item_t item;
	item.type = SENSOR_EVENT_ITEM;
	item.event = new_event;

	push_internal(item);
}

void csensor_event_queue::push(sensorhub_event_t const &event)
{
	sensorhub_event_t *new_event = new sensorhub_event_t;
	*new_event = event;

	sensor_event_queue_item_t item;
	item.type = SENSORHUB_EVENT_ITEM;
	item.event = new_event;

	push_internal(item);
}

void csensor_event_queue::push_internal(sensor_event_queue_item_t const &item)
{
	lock l(m_mutex);
	bool wake = m_queue.empty();

	if (m_queue.size() >= QUEUE_FULL_SIZE) {
		ERR("Queue is full");
	} else
		m_queue.push(item);

	if (wake)
		m_cond_var.notify_one();
}

sensor_event_queue_item_t csensor_event_queue::pop(void)
{
	ulock u(m_mutex);
	while (m_queue.empty())
		m_cond_var.wait(u);

	sensor_event_queue_item_t event = m_queue.front();
	m_queue.pop();
	return event;
}

