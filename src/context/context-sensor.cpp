/*
 * sf-plugin-common
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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/poll.h>

#include <common.h>
#include <cobject_type.h>
#include <cmutex.h>
#include <clist.h>
#include <cmodule.h>
#include <cpacket.h>
#include <cworker.h>
#include <csock.h>
#include <sf_common.h>
#include <linux/input.h>
#include <csensor_module.h>
#include <cconfig.h>

#include <context-sensor.h>
#include <sys/ioctl.h>

#define SENSORHUB_ID_CONTEXT 			(0)
#define SENSORHUB_ID_GESTURE			(1)

/* context */
#define EVENT_TYPE_CONTEXT_DATA         REL_RX
#define EVENT_TYPE_LARGE_CONTEXT_DATA   REL_RY
#define EVENT_TYPE_CONTEXT_NOTI         REL_RZ

#define SSPCONTEXT_DEVICE		"/dev/ssp_sensorhub"
#define SSP_INPUT_NODE_NAME			"ssp_context"
#define SENSORHUB_IOCTL_MAGIC		'S'
#define IOCTL_READ_LARGE_CONTEXT_DATA	_IOR(SENSORHUB_IOCTL_MAGIC, 3, char *)
/*****************************************************************************/

context_sensor::context_sensor()
: m_sensor_type(ID_CONTEXT)
, m_client(0)
, m_id(0x0000045A)
, m_version(1)
, m_enabled(false)
{
	m_pending_event.version = sizeof(sensorhub_event_t);
    m_pending_event.sensorhub = SENSORHUB_ID_CONTEXT;
    m_pending_event.type = SENSORHUB_TYPE_CONTEXT;
    memset(m_pending_event.hub_data, 0, sizeof(m_pending_event.hub_data));
    m_pending_event.hub_data_size = 0;

	m_node_handle = open_input_node(SSP_INPUT_NODE_NAME);

	if (m_node_handle < 0)
		throw ENXIO;

    if ((m_context_fd = open(SSPCONTEXT_DEVICE, O_RDWR)) < 0) {
        ERR("Open sensorhub device failed(%d)", m_context_fd);
		throw ENXIO;
    }

	INFO("m_context_fd = %s", SSPCONTEXT_DEVICE);

	INFO("context_sensor is created!\n");
}

context_sensor::~context_sensor()
{
	if (m_node_handle >= 0)
		close(m_node_handle);

	if(m_context_fd >= 0)
        close(m_context_fd);

	INFO("context_sensor is destroyed!\n");
}


int context_sensor::open_input_node(const char* input_node)
{
	int fd = -1;
	const char *dirname = "/dev/input";
	char devname[PATH_MAX];
	char *filename;
	DIR *dir;
	struct dirent *de;
	dir = opendir(dirname);

	if(dir == NULL)
		return -1;

	strcpy(devname, dirname);

	filename = devname + strlen(devname);
	*filename++ = '/';

	while((de = readdir(dir))) {
		if(de->d_name[0] == '.' &&
			(de->d_name[1] == '\0' ||
			(de->d_name[1] == '.' && de->d_name[2] == '\0')))
			continue;

		strcpy(filename, de->d_name);
		fd = open(devname, O_RDONLY);

		if (fd >=0) {
			char name[80];
			if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
				name[0] = '\0';
			}

			if (!strcmp(name, input_node)) {
				INFO("m_node_handle = %s", devname);
				break;
			} else {
				close(fd);
				fd = -1;
			}
		}
	}

	closedir(dir);

	if (fd < 0)
		ERR("couldn't find '%s' input device", input_node);

	return fd;
}


const char *context_sensor::name(void)
{
	return m_name.c_str();
}

int context_sensor::version(void)
{
	return m_version;
}

int context_sensor::id(void)
{
	return m_id;
}



bool context_sensor::is_data_ready(bool wait)
{
	bool ret;
	ret = update_value();
	return ret;
}

bool context_sensor::update_name(char *name)
{
	m_name = name;
	return true;
}



bool context_sensor::update_version(int version)
{
	m_version = version;
	return true;
}

bool context_sensor::update_id(int id)
{
	m_id = id;
	return true;
}

bool context_sensor::update_polling_interval(unsigned long val)
{
	return true;
}

bool context_sensor::start(void)
{
	AUTOLOCK(m_mutex);

	if (m_client > 0) {
		m_client++;
		INFO("Context sensor fake starting, client cnt = %d", m_client);
		return true;
	}

	m_enabled = true;
	m_client = 1;

	INFO("Context sensor real starting, client cnt = %d", m_client);
	return true;
}

bool context_sensor::stop(void)
{
	AUTOLOCK(m_mutex);

	INFO("Context sensor client cnt = %d\n", m_client);

	if (m_client > 1) {
		m_client--;
		INFO("Context sensor fake stopping, client cnt = %d", m_client);
		return true;
	}

	m_enabled = false;
	m_client = 0;
	INFO("Context sensor real stopping, client cnt = %d", m_client);
	return true;
}

int context_sensor::get_sensor_type(void)
{
	return m_sensor_type;
}

long context_sensor::set_cmd(int type , int property , long input_value)
{
	long value = -1;
	ERR("Cannot support any cmdn");
	return value;
}

int context_sensor::get_property(unsigned int property_level , void *property_data)
{
	return -1;
}

int context_sensor::get_struct_value(unsigned int struct_type, void *struct_values)
{
	retvm_if ((struct_values  == NULL), -1, "struct_values is NULL");

	if (struct_type == CONTEXT_BASE_DATA_SET) {
		sensorhub_event_t *sensorhub_event = NULL;

		sensorhub_event = (sensorhub_event_t *)struct_values;
		AUTOLOCK(m_event_mutex);
		*sensorhub_event = m_event;
		return 0;
	} else {
		ERR("Does not support type , struct_type : %d", struct_type);
	}
	return -1;

}

bool context_sensor::update_value(void)
{
	const int INPUT_MAX_BEFORE_SYN = 10;
	struct input_event context_input;
	int read_input_cnt = 0;
	bool syn = false;
	bool event_updated = false;

	fd_set readfds,exceptfds;

	FD_ZERO(&readfds);
	FD_ZERO(&exceptfds);

	FD_SET(m_node_handle, &readfds);
	FD_SET(m_node_handle, &exceptfds);

	int ret;
	ret = select(m_node_handle+1, &readfds, NULL, &exceptfds, NULL);

	if (ret == -1) {
		ERR("select error:%s m_node_handle:d", strerror(errno), m_node_handle);
		return false;
	} else if (!ret) {
		DBG("select timeout");
		return false;
	}

	if (FD_ISSET(m_node_handle, &exceptfds)) {
		ERR("select exception occurred!");
		return false;
	}

	if (FD_ISSET(m_node_handle, &readfds)) {
		INFO("context event detection!");

		while ((syn == false) && (read_input_cnt < INPUT_MAX_BEFORE_SYN)) {
			int input_len = read(m_node_handle, &context_input, sizeof(context_input));
			if (input_len != sizeof(context_input)) {
				ERR("context node read fail, read_len = %d", input_len);
				return false;
			}

			++read_input_cnt;

			int context_len;
			if (context_input.type == EV_REL) {
				float value = context_input.value;

				if (context_input.code == EVENT_TYPE_CONTEXT_DATA) {
					INFO("EVENT_TYPE_CONTEXT_DATA, hub_data_size=%d", value);
					m_pending_event.hub_data_size = value;
					context_len = read_context_data();

					if (context_len == 0)
						INFO("No library data");
					else if (context_len < 0)
						ERR("read_context_data() err(%d)", context_len);

				} else if (context_input.code == EVENT_TYPE_LARGE_CONTEXT_DATA) {
					INFO("EVENT_TYPE_LARGE_CONTEXT_DATA, hub_data_size=%d", value);
					m_pending_event.hub_data_size = value;
					context_len = read_large_context_data();

					if (context_len == 0)
						INFO("No large library data");
					else if (context_len < 0)
						ERR("read_large_context_data() err(%d)", context_len);

				} else if (context_input.code == EVENT_TYPE_CONTEXT_NOTI) {
					INFO("EVENT_TYPE_CONTEXT_NOTI, value=%d", value);
					context_len = 3;
					m_pending_event.hub_data_size = context_len;
					m_pending_event.hub_data[0] = 0x02;
					m_pending_event.hub_data[1] = 0x01;
					m_pending_event.hub_data[2] = value;
					print_context_data(__FUNCTION__, m_pending_event.hub_data, m_pending_event.hub_data_size);
				}
			} else if (context_input.type == EV_SYN) {
				syn = true;
				AUTOLOCK(m_mutex);
				if (m_enabled && (context_len > 0)) {
					AUTOLOCK(m_event_mutex);
					m_event = m_pending_event;
					event_updated = true;
					INFO("context event is received!");
				}
			} else {
				ERR("Unknown event (type=%d, code=%d)", context_input.type, context_input.code);
			}
		}

	} else {
		ERR("select nothing to read!!!");
		return false;
	}

	if (syn == false) {
		ERR("EV_SYN didn't come until %d inputs had come", read_input_cnt);
		return false;
	}

	return event_updated;
}


int context_sensor::print_context_data(const char* name, const char *data, int length)
{
    char buf[6];
    char *log_str;
    int log_size = strlen(name) + 2 + sizeof(buf) * length + 1;

    log_str = new char[log_size];
    memset(log_str, 0, log_size);

    for (int i = 0; i < length; i++ ) {
        if (i == 0) {
            strcat(log_str, name);
            strcat(log_str, ": ");
	} else {
            strcat(log_str, ", ");
	}
        sprintf(buf, "%d", (signed char)data[i]);
        strcat(log_str, buf);
    }

    INFO("%s", log_str);
    delete[] log_str;

    return length;
}


int context_sensor::send_sensorhub_data(const char* data, int data_len)
{
	return send_context_data(data, data_len);
}

int context_sensor::send_context_data(const char *data, int data_len)
{
    int ret;

    if (data_len <= 0) {
        ERR("Invalid data_len(%d)", data_len);
        return -EINVAL;
    }

    if (m_context_fd < 0) {
		ERR("Invalid context fd(%d)", m_context_fd);
		return -ENODEV;
    }

    print_context_data(__FUNCTION__, data, data_len);

write:
    ret = write(m_context_fd, data, data_len);
    if (ret < 0) {
        if (errno == EINTR) {
			INFO("EINTR! retry to write");
			goto write;
		}
		ERR("errno : %d , errstr : %s", errno, strerror(errno));
    }

    return ret < 0 ? -errno : ret;
}


int context_sensor::read_context_data(void)
{
    int ret = 0;

    if (m_context_fd < 0) {
		ERR("Invalid context fd(%d)", m_context_fd);
		return -ENODEV;
    }

read:
    ret = read(m_context_fd, m_pending_event.hub_data, m_pending_event.hub_data_size);
    if (ret > 0) {
		m_pending_event.hub_data_size = ret;
        print_context_data(__FUNCTION__, m_pending_event.hub_data, m_pending_event.hub_data_size);
    } else if (ret < 0) {
		if (errno == EINTR) {
		    DBG("EINTR! retry read");
			goto read;
		}

		ERR("errno : %d , errstr : %s", errno, strerror(errno));
		return -errno;
    }

    return ret;
}

int context_sensor::read_large_context_data(void)
{
    int ret = 0;

    if (m_context_fd < 0) {
		ERR("Invalid context fd(%d)", m_context_fd);
		return -ENODEV;
    }

ioctl:
    ret = ioctl(m_context_fd, IOCTL_READ_LARGE_CONTEXT_DATA, m_pending_event.hub_data);

	if (ret > 0) {
		m_pending_event.hub_data_size = ret;
        print_context_data(__FUNCTION__, m_pending_event.hub_data, m_pending_event.hub_data_size);
    } else if (ret < 0) {
		if (errno == EINTR) {
			INFO("EINTR! retry ioctl");
			goto ioctl;
		}

		ERR("errno : %d , errstr : %s", errno, strerror(errno));
		return -errno;
    }

    return ret;
}

cmodule *module_init(void *win, void *egl)
{
	context_sensor *sample;

	try {
		sample = new context_sensor();
	} catch (int ErrNo) {
		ERR("context_sensor class create fail , errno : %d , errstr : %s\n",ErrNo, strerror(ErrNo));
		return NULL;
	}

	return (cmodule*)sample;
}



void module_exit(cmodule *inst)
{
	context_sensor *sample = (context_sensor*)inst;
	delete sample;
}
