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
#include <cobject_type.h>
#include <cmutex.h>
#include <clist.h>
#include <cmodule.h>
#include <common.h>

cmodule *cmodule::m_head = NULL;
cmodule *cmodule::m_tail = NULL;
cmutex cmodule::m_lock;

cmodule::cmodule()
{
}

cmodule::~cmodule()
{
}

cmodule *cmodule::search_module(const char *name)
{
	cmodule *module;

	AUTOLOCK(m_lock);

	module = cmodule::m_head;
	while (module) {
		if (!strcmp(name, module->name())) {
			break;
		}

		module = (cmodule*)module->next();
	}

	return module;
}

cmodule *cmodule::register_module(const char *filename, void *win, void *egl)
{
	void *handle;
	cmodule *module;
	cmodule *(*init)(void *win, void *egl);
	void (*fini)(cmodule *inst);
	bool bstate;

	handle = dlopen(filename, RTLD_LAZY);
	if (!handle) {
		ERR("Target file is %s , dlerror : %s\n", filename , dlerror());
		return NULL;
	}


	fini = (void (*)(cmodule *))dlsym(handle, "module_exit");
	if (!fini) {
		ERR("Failed to find \"module_exit\" %s\n", dlerror());
		dlclose(handle);
		handle = NULL;
		return NULL;
	}

	init = (cmodule *(*)(void *, void*))dlsym(handle, "module_init");
	if (!init) {
		ERR("Failed to find \"module_init\" %s\n", dlerror());
		dlclose(handle);
		handle = NULL;
		return NULL;
	}


	module = init(win, egl);
	if (!module) {
		ERR("Failed to init the module => dlerror : %s , Target file is %s\n", dlerror(),filename);
		dlclose(handle);
		handle = NULL;
		return NULL;
	}

	module->m_filename = strdup(filename);
	if (!module->m_filename) {
		perror(__func__);
		dlclose(handle);
		handle = NULL;
		return NULL;
	}

	module->m_handle = handle;
	module->m_init = init;
	module->m_fini = fini;

	bstate = cmodule::add_to_list(module);
	if (!bstate) {
		ERR("Error cmodule::add_to_list fail , module : %p", module);
		return NULL;
	}
	return module;
}

bool cmodule::unregister_module(const char *name)
{
	bool bstate;

	if ( !name ) {
		ERR("Error invalid input name");
		return false;
	}

	cmodule *module;
	void *handle=NULL;

	module = cmodule::search_module(name);
	if (!module) {
		ERR("Unregistered module\n");
		return false;
	}
	if (module->m_handle) {
		DBG("This module is a base module of %s",module->m_filename);
		handle = module->m_handle;
	}

	free(module->m_filename);

	bstate = cmodule::del_from_list(module);
	if ( !bstate ) {
		ERR("Error cmodule::del_from_list fail , module : %p", module);
		return false;
	}

	if ( handle ) {
		dlclose(handle);
	}
	return true;
}

bool cmodule::unregister_module(cmodule *module)
{
	void *handle=NULL;
	bool bstate;
	if ( !module ) {
		ERR("Error invalid input module addr");
		return false;
	}
	if (module->m_handle) {
		DBG("This module is a base module of %s",module->m_filename);
		handle = module->m_handle;
	}

	free(module->m_filename);

	bstate = cmodule::del_from_list(module);
	if ( !bstate ) {
		ERR("Error cmodule::del_from_list fail , module : %p", module);
		return false;
	}

	if ( handle ) {
		dlclose(handle);
	}
	return true;
}

bool cmodule::add_to_list(cmodule *module)
{
	if ( !module ) {
		ERR("Error invalid input module addr");
		return false;
	}

	AUTOLOCK(m_lock);
	if (cmodule::m_head == NULL && cmodule::m_tail == NULL) {
		cmodule::m_head = cmodule::m_tail = module;
	} else {
		if (module->link(AFTER, cmodule::m_tail)) {
			cmodule::m_tail = module;
		}
	}
	return true;
}

bool cmodule::del_from_list(cmodule *module)
{
	if ( !module ) {
		ERR("Error invalid input module addr");
		return false;
	}

	AUTOLOCK(m_lock);
	module->unlink();

	delete module;
	return true;
}

unsigned long long cmodule::get_timestamp(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return ((unsigned long long)(t.tv_sec)*1000000000LL + t.tv_nsec) / 1000;
}

unsigned long long cmodule::get_timestamp(timeval *t)
{
	if (!t) {
		ERR("t is NULL");
		return 0;
	}

	return ((unsigned long long)(t->tv_sec)*1000000LL +t->tv_usec);
}

//! End of a file
