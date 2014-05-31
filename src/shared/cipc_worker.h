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

#if !defined(_CIPC_WORKER_CLASS_H_)
#define _CIPC_WORKER_CLASS_H_
#include <pthread.h>

class cipc_worker
{
public:
	//! These values are used for the index of function array,
	enum ipc_worker_state_s {
		STOPPED	= 0x00,
		START	= 0x01,
		STARTED	= 0x02,
		STOP	= 0x03,
		TERMINATE = 0x04,
		ENUM_LAST = 0x05,
	};

	cipc_worker(void);

	bool start(void);
	bool stop(void);
	bool terminate(void);
	bool m_main(void);

	ipc_worker_state_s state(void);

	void set_start(void *(*start)(void *data));
	void set_started(void *(*started)(void *data));
	void set_stop(void *(*stopped)(void *data));
	void set_terminate(void *(*terminate)(void *));

	void set_context(void *ctx);

	typedef void *(*function_t)(void *arg);

private:
	virtual ~cipc_worker(void);

	ipc_worker_state_s m_state;
	pthread_t m_thid;

	void *m_context;

	function_t m_func[ENUM_LAST];

	static void *started(void *data);

	pthread_mutex_t mutex_lock;
};

#endif
//! End of a file
