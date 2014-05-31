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

#include <cevent_socket.h>
#include <attr/xattr.h>
#include <sys/stat.h>


cevent_socket::cevent_socket()
: m_sock_fd(-1)
{
	memset(&m_addr,0,sizeof(m_addr));
}

cevent_socket::cevent_socket(const cevent_socket &sock)
{
	m_sock_fd = sock.m_sock_fd;
	memcpy(&m_addr, &sock.m_addr, sizeof(sockaddr_un));
}

cevent_socket::~cevent_socket() { }

bool cevent_socket::create(void)
{
	m_sock_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);

	if (!is_valid()) {
		ERR("Failed to create event socket for %s, errno : %d , errstr : %s ",
			get_client_name(), errno, strerror(errno));
		return false;
	}

	return true;
}

bool cevent_socket::bind (const char *sock_path)
{
	int length;
	mode_t socket_mode;

	if (!is_valid()) {
		ERR("%s's event socket is invalid", get_client_name());
		return false;
	}

	if((fsetxattr(m_sock_fd, "security.SMACK64IPOUT", "@", 2, 0)) < 0) {
		if(errno != EOPNOTSUPP) {
			close();
			ERR("security.SMACK64IPOUT error = [%d][%s]\n", errno, strerror(errno) );
			return false;
		}
	}

	if((fsetxattr(m_sock_fd, "security.SMACK64IPIN", "*", 2, 0)) < 0) {
		if(errno != EOPNOTSUPP)	{
			close();
			ERR("security.SMACK64IPIN error  = [%d][%s]\n", errno, strerror(errno) );
			return false;
		}
	}

	if (!access(sock_path, F_OK)) {
		unlink(sock_path);
	}

	m_addr.sun_family = AF_UNIX;
	strcpy(m_addr.sun_path, sock_path);

	length = strlen(m_addr.sun_path) + sizeof(m_addr.sun_family);

	if (::bind(m_sock_fd, (struct sockaddr *)&m_addr, length) < 0) {
		ERR("Binding failed for event channel, errno : %d , errstr : %s", errno, strerror(errno));
		close();
		return false;
	}

	socket_mode = ( S_IRWXU | S_IRWXG | S_IRWXO );
	if (chmod(sock_path, socket_mode) < 0) {
		ERR("chmod failed for event channel, errno : %d , errstr : %s", errno, strerror(errno));
		close();
		return false;
	}

	return true;
}

bool cevent_socket::listen(const int max_connections)
{
	if (!is_valid()) {
		ERR("Event socket is invalid");
		return false;
	}

	if (::listen(m_sock_fd, max_connections) < 0) {
		ERR("Listening event channel failed, errno : %d , errstr : %s", errno, strerror(errno));
		close();
		return false;
	}

	return true;
}

bool cevent_socket::accept(cevent_socket& client_socket) const
{
	int addr_length = sizeof(m_addr);
	client_socket.m_sock_fd = ::accept(m_sock_fd, (sockaddr *)&m_addr, (socklen_t *)&addr_length);

	if (!client_socket.is_valid()) {
		ERR("Accept event channel failed, errno : %d , errstr : %s", errno, strerror(errno));
		return false;
	}

	return true;
}

ssize_t cevent_socket::send(void const* buffer, size_t size)
{
	ssize_t err, len;

	do {
		len = ::send(m_sock_fd, buffer, size, m_send_flags);
		err = len < 0 ? errno : 0;
	} while (err == EINTR);

	if (err) {
		ERR("send(%d, 0x%x, %d, 0x%x) = %d cause = %s(%d)",
			m_sock_fd, buffer, size, m_send_flags, len, strerror(errno), errno);
	}

	return err == 0 ? len : -err;
}

ssize_t cevent_socket::recv(void* buffer, size_t size)
{
    ssize_t err, len;

	do {
        len = ::recv(m_sock_fd, buffer, size, m_recv_flags);
        err = len < 0 ? errno : 0;
    } while (err == EINTR);

	if ((err == EAGAIN) || (err == EWOULDBLOCK)) {
		DBG("recv(%d, 0x%x, %d, 0x%x) = %d cause = %s(%d)",
			m_sock_fd, buffer, size, m_recv_flags, len, strerror(errno), errno);
		return 0;
	}

	if (err) {
		ERR("recv(%d, 0x%x, %d, 0x%x) = %d cause = %s(%d)",
			m_sock_fd, buffer, size, m_recv_flags, len, strerror(errno), errno);
	}

    return err == 0 ? len : -err;
}

bool cevent_socket::connect(const char *sock_path)
{
	const int TIMEOUT = 3;
	fd_set write_fds;
	struct timeval tv;
	int addr_len;
	bool prev_blocking_mode;

	if (!is_valid()) {
		ERR("%s's event chaanel socket is invalid", get_client_name());
		return false;
	}

	prev_blocking_mode = is_blocking_mode();

	set_blocking_mode(false);

	m_addr.sun_family = AF_UNIX;
	strcpy(m_addr.sun_path, sock_path);
	addr_len = strlen(m_addr.sun_path) + sizeof(m_addr.sun_family);

	if (::connect(m_sock_fd,(sockaddr *) &m_addr, addr_len) < 0) {
		ERR("connect error: %s sock_fd: %d\n for %s", strerror(errno), m_sock_fd, get_client_name());
		return false;
	}

	FD_ZERO(&write_fds);
	FD_SET(m_sock_fd, &write_fds);
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;

	int ret;

	ret = select(m_sock_fd + 1, NULL, &write_fds, NULL, &tv);

	if (ret == -1) {
		ERR("select error: %s sock_fd: %d\n for %s", strerror(errno), m_sock_fd, get_client_name());
		close();
		return false;
	} else if (!ret) {
		ERR("select timeout: %d seconds elapsed for %s", tv.tv_sec, get_client_name());
		close();
		return true;
	}

	if (!FD_ISSET(m_sock_fd, &write_fds)) {
		ERR("select failed for %s, nothing to write, m_sock_fd : %d", get_client_name(), m_sock_fd);
		close();
		return false;
	}

	int so_error;
	socklen_t len = sizeof(so_error);

	if (getsockopt(m_sock_fd, SOL_SOCKET, SO_ERROR, &so_error, &len) == -1) {
		ERR("getsockopt failed for %s, m_sock_fd : %d, errno : %d , errstr : %s",
			get_client_name(), m_sock_fd, errno, strerror(errno));
		close();
		return false;
	}

	if (so_error) {
		ERR("SO_ERROR occurred for %s, m_sock_fd : %d, so_error : %d",
			get_client_name(), m_sock_fd, so_error);
		close();
		return false;
	}

	if (prev_blocking_mode)
		set_blocking_mode(true);

	return true;
}

bool cevent_socket::set_blocking_mode(bool blocking)
{
	int flags;

	flags = fcntl(m_sock_fd, F_GETFL);

	if (flags == -1) {
		ERR("fcntl(F_GETFL) failed for %s, m_sock_fd: %d, errno : %d , errstr : %s", get_client_name(), m_sock_fd, errno, strerror(errno));
		return false;
	}

	flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);

	flags = fcntl(m_sock_fd, F_SETFL, flags);

	if (flags == -1) {
		ERR("fcntl(F_SETFL) failed for %s, m_sock_fd: %d, errno : %d , errstr : %s", get_client_name(), m_sock_fd, errno, strerror(errno));
		return false;
	}

	return true;
}

bool cevent_socket::set_connection_mode(void)
{
	struct timeval tv;
	const int TIMEOUT = 3;

	set_blocking_mode(true);

	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;

	if(setsockopt(m_sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		ERR("Set SO_RCVTIMEO failed for %s, m_sock_fd : %d, errno : %d , errstr : %s",
			get_client_name(), m_sock_fd, errno, strerror(errno));
		close();
		return false;
	}

	m_send_flags = MSG_NOSIGNAL;
	m_recv_flags = MSG_NOSIGNAL;

	return true;
}

bool cevent_socket::set_transfer_mode(void)
{
	set_blocking_mode(false);


	m_send_flags = MSG_DONTWAIT | MSG_NOSIGNAL;
	m_recv_flags = MSG_DONTWAIT | MSG_NOSIGNAL;

	return true;
}

bool cevent_socket::is_blocking_mode(void)
{
	int flags;

	flags = fcntl(m_sock_fd, F_GETFL);

	if (flags == -1) {
		ERR("fcntl(F_GETFL) failed for %s, m_sock_fd: %d, errno : %d , errstr : %s", get_client_name(), m_sock_fd, errno, strerror(errno));
		return false;
	}

	return !(flags & O_NONBLOCK);
}

bool cevent_socket::close(void)
{
	if (m_sock_fd >= 0) {
		if (::close(m_sock_fd) < 0) {
			ERR("event socket close failed, errno : %d , errstr : %s", errno, strerror(errno));
			return false;
		}
		m_sock_fd = -1;
	}

	return true;
}
