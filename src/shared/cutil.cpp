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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <cobject_type.h>
#include <cutil.h>
#include "common.h"

cutil::cutil(void)
: ctype(ctype::UTIL)
{
}

cutil::~cutil(void)
{
}

unsigned long cutil::string_hash(char *name)
{
	register unsigned long ret = 0;
	int len;

	len = strlen(name);
	while (--len >= 0) {
		ret += (unsigned long)name[len];
	}

	return ret;
}

bool cutil::change_to_daemon(void)
{
	pid_t pid;
	pid = fork();

	if (pid < 0) {
		return false;
	}

	if (pid != 0) {
		exit(0);
	}

	close(0);
	close(1);
	close(2);
	setsid();
	return true;
}

char *cutil::ltrim(char *str)
{
	unsigned int index = 0;

	while ((str[index] == ' ')||(str[index] == '\t')) index ++;
	return &str[index];
}

int cutil::rtrim(char *str)
{
	int len;
	int org_len;

	org_len = len = strlen(str) - 1;

	while ((str[len] == ' ')||(str[len] == '\t')) len --;
	str[len + 1] = (char)NULL;

	return org_len - len;
}

//! End of a file
