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
#include <cworker.h>
#include "common.h"

cworker::cworker(void)
	: m_state(INITIAL)
	, m_context(NULL)
	, mutex_lock(PTHREAD_MUTEX_INITIALIZER)
	  , th_cond(PTHREAD_COND_INITIALIZER)
{
	register int i;
	int ret;
	for (i = 0; i < ENUM_LAST; i ++) {
		m_func[i] = NULL;
	}

	ret = pthread_mutex_init(&mutex_lock, NULL);
	if (ret != 0) {
		ERR("pthread_mutex_init : %s",strerror(errno));
		throw EINVAL;
	}

	ret = pthread_cond_init(&th_cond, NULL);
	if (ret != 0) {
		ERR("pthread_cond_init : %s",strerror(errno));
		throw EINVAL;
	}
	DBG("processor worker created\n");
}

cworker::~cworker(void)
{
	DBG("----------Processor WORKER TERMINATED--------\n");

	pthread_mutex_lock(&(mutex_lock));

	m_state = TERMINATE;
	pthread_cond_signal(&th_cond);

	pthread_mutex_unlock(&(mutex_lock));

	pthread_cond_destroy(&th_cond);

	if (m_func[TERMINATE])
		m_func[TERMINATE](m_context);

	DBG("processor worker terminated\n");
}

bool cworker::start(void)
{
	int ret = 0;

	pthread_mutex_lock(&(mutex_lock));

	if(m_state == INITIAL)
	{
		pthread_mutex_unlock(&(mutex_lock));
		ret = pthread_create(&m_thid, NULL, started, this);

		if(ret != 0)
		{
			pthread_mutex_lock(&(mutex_lock));
			m_state = INITIAL;
			pthread_mutex_unlock(&(mutex_lock));
			ERR("thread create fail\n");
			return false;
		}
		else
		{
			pthread_detach(m_thid);
		}

		pthread_mutex_lock(&(mutex_lock));
	}
	else if ((m_state == START) || (m_state == STARTED)) {
		INFO("Already started m_state =%d\n", m_state);
		pthread_mutex_unlock(&(mutex_lock));
		return false;
	}

	ret = pthread_cond_signal(&th_cond);
	if (ret != 0) {
		ERR("pthread_cond_wait : %s",strerror(errno));
	}

	m_state = START;
	pthread_mutex_unlock(&(mutex_lock));

	return true;
}

bool cworker::terminate(void)
{
	pthread_mutex_lock(&(mutex_lock));
	m_state = TERMINATE;
	pthread_mutex_unlock(&(mutex_lock));
	delete this;
	return true;
}

void *cworker::started(void *data)
{
	cworker *inst = (cworker*)data;
	worker_state_s state = STOPPED;

	do
	{
		state = (worker_state_s)(int)inst->m_func[STARTED](inst->m_context);

		if(state == STOPPED || inst->m_state == STOP)
			inst->stopped();

	}while(state != TERMINATE);

	return NULL;
}

bool cworker::stop(void)
{
	pthread_mutex_lock(&(mutex_lock));
	if (m_state == STOP) {
		ERR("Already stopped\n");
		pthread_mutex_unlock(&(mutex_lock));
		return false;
	}
	m_state = STOP;
	pthread_mutex_unlock(&(mutex_lock));

	return true;
}

bool cworker::stopped(void)
{
	pthread_mutex_lock(&mutex_lock);
	pthread_cond_wait(&th_cond, &mutex_lock);
	pthread_mutex_unlock(&mutex_lock);
	return true;
}

cworker::worker_state_s cworker::state(void)
{
	return m_state;
}

void cworker::set_start(void *(*start)(void *data))
{
	m_func[START] = start;
}

void cworker::set_started(void *(*started)(void *data))
{
	m_func[STARTED] = started;
}

void cworker::set_stop(void *(*stop)(void *data))
{
	m_func[STOP] = stop;
}

void cworker::set_stopped(void *(*stopped)(void *data))
{
	m_func[STOPPED] = stopped;
}

void cworker::set_terminate(void *(*term)(void *data))
{
	m_func[TERMINATE] = term;
}

void cworker::set_context(void *ctx)
{
	m_context = ctx;
}

//! End of a file
