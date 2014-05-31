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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <attr/xattr.h>
#include <pthread.h>
#include <cobject_type.h>
#include <cmutex.h>
#include <clist.h>
#include <cipc_worker.h>
#include <csock.h>
#include "common.h"

extern int errno;
const int MAX_CONNECT = 10;
const int DELAY_FOR_CONNECT = 10000;

csock::csock(int handle)
: m_handle(handle)
, m_name(NULL)
, m_client_ctx(NULL)
, m_on_close(true)
{
	int len;

	m_start = NULL;
	m_running = NULL;
	m_stop = NULL;

	len = sizeof(m_addr);
	bzero(&m_addr, len);
}

csock::csock(char *name, int port, bool server)
: m_handle(-1)
, m_client_ctx(NULL)
, m_on_close(true)
{
	int len;
	int domain;
	sockaddr *sock_ptr = NULL;
	struct timeval tv;

	m_start = NULL;
	m_running = NULL;
	m_stop = NULL;

	if (name) {
		m_name = strdup(name);
		if (!m_name) {
			ERR("Failed to allocate memory for csock name \n");
			throw EFAULT;
		}
	} else {
		m_name = NULL;
	}

	if (m_name) {
		len = sizeof(m_addr);
		bzero(&m_addr, len);

		if (server && !access(m_name, F_OK)) {
			unlink(m_name);
		}

		if ( sizeof(m_name) > (sizeof(m_addr.sun_path) -1) ) {
			ERR("Too long input path name\n");
			throw EINVAL;
		} else {
			strncpy(m_addr.sun_path, m_name, sizeof(m_addr.sun_path) -1 );
		}

		m_addr.sun_family = AF_UNIX;
		domain = AF_UNIX;

		sock_ptr = (sockaddr*)&m_addr;
	} else {
		ERR("Invalid argument (name == NULL)\n");
		throw EINVAL;
	}

	m_handle = socket(domain, SOCK_STREAM, 0);
	if (m_handle < 0) {
		ERR("socket fail : return value : %d\n",m_handle);
		m_handle = -1;
		throw EFAULT;
	}

	if (server) {
		int state;

		if(getuid() == 0)
		{
			if((fsetxattr(m_handle, "security.SMACK64IPOUT", "@", 2, 0)) < 0 )
			{
				if(errno != EOPNOTSUPP)
				{
					m_handle = -1;
					ERR("security.SMACK64IPOUT error = [%d][%s]\n", errno, strerror(errno) );
					throw EFAULT;
				}
			}
			if((fsetxattr(m_handle, "security.SMACK64IPIN", "*", 2, 0)) < 0 )
			{
				if(errno != EOPNOTSUPP)
				{
					m_handle = -1;
					ERR("security.SMACK64IPIN error  = [%d][%s]\n", errno, strerror(errno) );
					throw EFAULT;
				}
			}
		}

		state = bind(m_handle, sock_ptr, len);
		if (state < 0) {
			ERR("bind fail : return value : %d\n",state );
			close(m_handle);
			m_handle = -1;
			throw EFAULT;
		}

		if (m_name) {
			mode_t socket_mode;

			socket_mode = ( S_IRWXU | S_IRWXG | S_IRWXO );
			state = chmod(m_name,socket_mode);
			if (state < 0) {
				ERR("chmod fail : return value : %d\n",state );
				close(m_handle);
				m_handle = -1;
				throw EFAULT;
			}
		}

		state = listen(m_handle, 5);
		if (state < 0) {
			ERR("listen fail\n");
			close(m_handle);
			m_handle = -1;
			throw EFAULT;
		}
	}
	else
	{
		tv.tv_sec = 3;
		tv.tv_usec = 0;

		if( setsockopt(m_handle, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0)
			ERR("setsockopt fail");
	}
}

csock::~csock(void)
{
	if (m_handle >= 0 && m_on_close) close(m_handle);
	if (m_name) free(m_name);
}
bool csock::connect_to_server(void)
{
	int i = 0;

	int len;
	sockaddr *sock_ptr = NULL;

	len = sizeof(m_addr);
	sock_ptr = (struct sockaddr*)&m_addr;

	for (i = 0; i < MAX_CONNECT; i++) {
		if (connect(m_handle, sock_ptr, len) == 0) {
			return true;
		} else {
			DBG("wait for accept worker");
			usleep(DELAY_FOR_CONNECT);
		}
	}

	ERR("connect fail , m_handle : %d , sock_ptr : %p , len : %d ,%s\n",m_handle , sock_ptr , len, strerror(errno));
	close(m_handle);
	m_handle = -1;
	return false;
}

bool csock::wait_for_client(void)
{
	int ret = 0;

	ret = pthread_create(&m_thid, NULL, thread_tcp_main, this);
	if(ret != 0) {
		ERR("accept worker creation fail\n");
		return false;
	} else {
		DBG("thread for thread_tcp_main created\n");
		pthread_detach(m_thid);
	}

	DBG("Wait for a client\n");
	return true;
}

void *csock::thread_tcp_main(void *data)
{
	csock *ipc = (csock*)data;
	int client_handle;
	socklen_t client_len;
	sockaddr_un clientaddr_un;
	thread_arg_t *arg;
	sockaddr *clientaddr;

	DBG("IPC thread enabled\n");
	client_len = sizeof(clientaddr_un);
	clientaddr = (sockaddr*)&clientaddr_un;

	while (true) {
		DBG("Wait for a client - handle %d\n", ipc->m_handle);
		client_handle = accept(ipc->m_handle, clientaddr, &client_len);
		if (client_handle < 0) {
			perror("accept");
			ERR("Accept fail ,client_handle : %d\n", client_handle);
			close(ipc->m_handle);
			ipc->m_handle = -1;
			return NULL;
		}

		if (client_handle == 0) {
			ERR("Strange handle detected , client_handle : %d\n", client_handle);
		}

		DBG("New client connected (handle : %d), func : %p\n", client_handle, ipc->m_running);
		try {
			arg = new thread_arg_t;
		} catch (...) {
			ERR("fail to new thread_arg_t\n");
			close(client_handle);
			return NULL;
		}

		arg->client_handle = client_handle;
		arg->sock = ipc;
		arg->client_ctx = NULL;

		try {
			arg->worker = new cipc_worker();
		} catch (...) {
			ERR("fail to new cipc_worker\n");
			close(client_handle);
			delete arg;
			return NULL;
		}

		DBG("Start new worker for handle %d\n", arg->client_handle);
		arg->worker->set_context(arg);
		arg->worker->set_start(arg->sock->m_start);
		arg->worker->set_started(arg->sock->m_running);
		arg->worker->set_stop(arg->sock->m_stop);
		arg->worker->set_terminate(csock::worker_terminate);
		if(arg->worker->start() == false)
		{
			close(arg->client_handle);
			delete arg;
		}
	}

	return NULL;
}

void *csock::worker_terminate(void *data)
{
	thread_arg_t *arg = (thread_arg_t *)data;

	DBG("Worker is finished\n");
	close(arg->client_handle);
	delete arg;
	return (void*)NULL;
}

void csock::set_worker(void *(*start)(void *data), void *(*running)(void *data), void *(*stop)(void *data))
{
	m_start = start;
	m_running = running;
	m_stop = stop;
}

bool csock::recv(void *buffer, int size)
{
	ssize_t recv_size;
	int total_recv_size = 0;
	const int chance = 10;
	int retry = 0;

	DBG("Recv message : data size is %d\n", size);
	if (m_handle < 0) {
		ERR("Invalid handle\n");
		return false;
	}

	if (size == 0) {
		DBG("Try to read ZERO, just return true\n");
		return true;
	}

	if (size < 0) {
		ERR("invalid size of packet");
		return false;
	}


	do {
		recv_size = ::recv(m_handle, (char*)buffer + total_recv_size, size - total_recv_size, MSG_NOSIGNAL);

		if (recv_size > 0) {
			total_recv_size += recv_size;
		}
		else if (recv_size == 0) {
			ERR("recv(%d, 0x%p + %d, %d - %d) = %d, beacuse the peer performed shutdown!",
				m_handle, buffer, total_recv_size, size, total_recv_size, recv_size);
			close(m_handle);
			m_handle = -1;
			return false;
		} else {
			ERR("recv(%d, 0x%p + %d, %d - %d) = %d, cause = %s(%d)", m_handle, buffer,
				total_recv_size, size, total_recv_size, recv_size,  strerror(errno), errno);

			if (errno != EINTR) {
				close(m_handle);
				m_handle = -1;
				return false;
			}
		}

		++retry;
	} while ((total_recv_size < size) && (retry < chance));

	if (total_recv_size != size) {
		ERR("After retrying recv() % times, it fails", retry);
		close(m_handle);
		m_handle = -1;
		return false;
	}
	return true;
}

bool csock::send(void *buffer, int size)
{
	int sent_size = 0;
	int total_sent_size = 0;
	const int chance = 10;
	int retry = 0;

	DBG("Send message : data size is %d\n", size);
	if (size == 0) {
		return true;
	}

	if (size < 0) {
		ERR("invalid size of packet");
		return false;
	}

	if (m_handle < 0) {
		ERR("Invalid handle\n");
		return false;
	}

	do {
		sent_size = ::send(m_handle, (char*)buffer + total_sent_size, size - total_sent_size, MSG_NOSIGNAL);

		if (sent_size >= 0) {
			total_sent_size += sent_size;
		} else {
			ERR("send(%d, 0x%p + %d, %d - %d) = %d, cause = %s(%d)", m_handle, buffer,
				total_sent_size, size, total_sent_size, sent_size, strerror(errno), errno);

			if (errno != EINTR) {
				close(m_handle);
				m_handle = -1;
				return false;
			}
		}

		++retry;
	} while ((total_sent_size < size) && (retry < chance));

	if (total_sent_size != size) {
		ERR("After retrying send() % times, it fails", retry);
		close(m_handle);
		m_handle = -1;
		return false;
	}

	return true;
}

void csock::set_on_close(bool flag)
{
	m_on_close = flag;
}

int csock::handle(void)
{
	return m_handle;
}

bool csock::disconnect(void)
{
	if (m_handle >= 0) {
		close(m_handle);
		m_handle = -1;
		return true;
	}

	return false;
}

//! End of a file
