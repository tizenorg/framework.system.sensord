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
#include <stdlib.h>
#include <errno.h>
#include <dlfcn.h>
#include <pthread.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/time.h>
#include <cobject_type.h>
#include <cmutex.h>
#include <clist.h>
#include <cmodule.h>
#include "common.h"
#include <cpacket.h>
#include <cworker.h>
#include <cipc_worker.h>
#include <csock.h>
#include <sf_common.h>
#include <csensor_module.h>

csensor_module::csensor_module()
{
	ctype::set_type(SF_PLUGIN_SENSOR);
}

csensor_module::~csensor_module()
{
}

void csensor_module::lock(void)
{
	m_mutex.lock();
}

void csensor_module::unlock(void)
{
	m_mutex.unlock();
}

int csensor_module::send_sensorhub_data( const char* data, int data_len)
{
	return -1;
}

//! End of a file
