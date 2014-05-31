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

#ifndef CEVENT_SOCKET_H_
#define CEVENT_SOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "common.h"
#include <string>
using std::string;

class cevent_socket {
public:
	cevent_socket();
	virtual ~cevent_socket();
	cevent_socket(const cevent_socket &sock);

	//Server
	bool create(void);
	bool bind (const char *sock_path);
	bool listen(const int max_connections);
	bool accept(cevent_socket& client_socket) const;

	//Client
	bool connect(const char *sock_path);

	//Data Transfer
	ssize_t send(void const* buffer, size_t size);
	ssize_t recv(void* buffer, size_t size);

	bool set_connection_mode(void);
	bool set_transfer_mode(void);
	bool is_blocking_mode(void);

	//check if socket is created
	bool is_valid(void) const { return (m_sock_fd >= 0); }
	int get_socket_fd(void) const { return m_sock_fd; }

	bool close(void);

	bool is_block_mode(void);

private:
	bool set_blocking_mode(bool blocking);

	int m_sock_fd;
	sockaddr_un m_addr;
	int m_send_flags;
	int m_recv_flags;
};

#endif /* CEVENT_SOCKET_H_ */
