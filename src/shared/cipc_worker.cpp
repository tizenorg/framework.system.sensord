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
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <cobject_type.h>
#include <clist.h>
#include <cmutex.h>
#include <cipc_worker.h>
#include "common.h"

cipc_worker::cipc_worker(void)
: m_state(STOP)
, m_context(NULL)
, mutex_lock(PTHREAD_MUTEX_INITIALIZER)
{
	register int i;

	for (i = 0; i < ENUM_LAST; i ++) {
		m_func[i] = NULL;
	}
}

cipc_worker::~cipc_worker(void)
{
	DBG("-----------Client WORKER TERMINATED------------\n");

	if (m_func[TERMINATE])
		m_func[TERMINATE](m_context);

	m_state = TERMINATE;

	DBG("terminated cipc_worker",pthread_self());
}

bool cipc_worker::start(void)
{
	ipc_worker_state_s state;
	int ret = 0;

	DBG("Client worker start\n");

	pthread_mutex_lock(&(mutex_lock));
	m_state = START;
	pthread_mutex_unlock(&(mutex_lock));

	state = (ipc_worker_state_s)(int)m_func[START](m_context);

	if (state == TERMINATE) {
		pthread_mutex_lock(&(mutex_lock));
		m_state = TERMINATE;
		pthread_mutex_unlock(&(mutex_lock));
		delete this;
		return false;
	}
	else
	{
		ret = pthread_create(&m_thid, NULL, started, this);

		if(ret != 0)
		{
			pthread_mutex_lock(&(mutex_lock));
			m_state = STOP;
			pthread_mutex_unlock(&(mutex_lock));
			ERR("thread create fail\n");
			return false;
		}
		else
		{
			ret = pthread_detach(m_thid);

			if(ret != 0)
			{
				ERR("thread detach fail\n");
				return false;
			}
			else
			{
				DBG("thread creation for Client worker END\n");
			}
		}
	}

	return true;
}

bool cipc_worker::terminate(void)
{
	pthread_mutex_lock(&(mutex_lock));
	m_state = TERMINATE;
	pthread_mutex_unlock(&(mutex_lock));
	return true;
}

void *cipc_worker::started(void *data)
{
	cipc_worker *inst = (cipc_worker*)data;
	ipc_worker_state_s state;

	do
	{
		state = (ipc_worker_state_s)(int)inst->m_func[STARTED](inst->m_context);
		if(state == TERMINATE)
		{
			pthread_mutex_lock(&(inst->mutex_lock));
			inst->m_state = TERMINATE;
			pthread_mutex_unlock(&(inst->mutex_lock));
			inst->m_func[STOP](inst->m_context);
			delete inst;
			return NULL;
		}
	}while(state == START && inst->m_state == START);

	DBG("\n\n\n############Client worker thread END############\n\n\n");
	pthread_mutex_lock(&(inst->mutex_lock));
	inst->m_state = STOPPED;
	pthread_mutex_unlock(&(inst->mutex_lock));

	delete inst;
	return NULL;
}

bool cipc_worker::stop(void)
{
	ipc_worker_state_s state;

	state = (ipc_worker_state_s)(int)m_func[STOP](m_context);

	pthread_mutex_lock(&(mutex_lock));
	m_state = TERMINATE;
	pthread_mutex_unlock(&(mutex_lock));
	return true;
}

cipc_worker::ipc_worker_state_s cipc_worker::state(void)
{
	return m_state;
}

void cipc_worker::set_start(void *(*start)(void *data))
{
	m_func[START] = start;
}

void cipc_worker::set_started(void *(*started)(void *data))
{
	m_func[STARTED] = started;
}

void cipc_worker::set_stop(void *(*stop)(void *data))
{
	m_func[STOP] = stop;
}

void cipc_worker::set_terminate(void *(*term)(void *data))
{
	m_func[TERMINATE] = term;
}

void cipc_worker::set_context(void *ctx)
{
	m_context = ctx;
}

//! End of a file
