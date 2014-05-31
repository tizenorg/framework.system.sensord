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

#if !defined(_CSOCK_CLASS_H_)
#define _CSOCK_CLASS_H_
#include <cipc_worker.h>
#include <cmutex.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

class csock : public cmutex
{
public:
	struct thread_arg_t {
		csock *sock;
		int client_handle;
		cipc_worker *worker;
		void *client_ctx;
	};

	csock(int handle);
	csock(char *name, int port, bool server = false);
	virtual ~csock(void);

	bool connect_to_server(void);
	bool wait_for_client(void);

	void set_on_close(bool on_close = true);

	void set_worker(void *(*start)(void *data), void *(*running)(void *data), void *(*stop)(void *data));

	bool recv(void *buffer, int len);
	bool send(void *buffer, int len);

	int handle(void);

	bool disconnect(void);

	static void *thread_tcp_main(void *arg);

	static void *worker_terminate(void *data);

private:
	int m_handle;
	char *m_name;

	sockaddr_un m_addr;

	void *(*m_start)(void *data);
	void *(*m_running)(void *data);
	void *(*m_stop)(void *data);

	void *m_client_ctx;
	pthread_t m_thid;
	bool m_on_close;
	int m_mode;
};

#endif
//! End of a file
