/*
 *  sensor-framework
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: JuHyun Kim <jh8212.kim@samsung.com>
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
#include <errno.h>
#include <dlfcn.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/un.h>
#include <common.h>
#include <cobject_type.h>
#include <clist.h>
#include <ccatalog.h>
#include <cmutex.h>
#include <cmodule.h>
#include <cpacket.h>
#include <cworker.h>
#include <cipc_worker.h>
#include <csock.h>
#include <ctrim.h>
#include <sf_common.h>
#include <csensor_module.h>
#include <cfilter_module.h>
#include <cprocessor_module.h>
#include <cdata_stream.h>
#include <resource_str.h>
#include <csensor_usage.h>

#define PROCESSOR_NAME_LENGTH 50

cdata_stream *cdata_stream::m_head = NULL;
cdata_stream *cdata_stream::m_tail = NULL;

ccatalog cdata_stream::m_catalog;

cdata_stream::cdata_stream()
: m_filter_head(NULL)
, m_filter_tail(NULL)
, m_processor_head(NULL)
, m_processor_tail(NULL)
, m_name(NULL)
, m_id(0)
, m_client(0)
, mutex_lock(PTHREAD_MUTEX_INITIALIZER)
{
	if (!m_head) {
		m_head = m_tail = this;
	} else {
		if(!clist::link(clist::AFTER, m_tail))
			m_head = m_tail = this;
		else
			m_tail = this;
	}
	ctype::set_type(SF_DATA_STREAM);
}

cdata_stream::~cdata_stream()
{
	cdata_stream *prev;
	cdata_stream *next;
	processor_list_t *processor;
	processor_list_t *next_processor;
	filter_list_t *filter;
	filter_list_t *next_filter;

	prev = (cdata_stream*)clist::prev();
	next = (cdata_stream*)clist::next();

	if (m_tail == this) {
		m_tail = next;
	}

	if (m_head == this) {
		m_head = prev;
	}

	clist::unlink();

	processor = m_processor_head;
	while (processor) {
		next_processor = (processor_list_t*)processor->next();

		processor->unlink();
		processor->processor->destroy(processor->processor);
		delete processor;

		processor = next_processor;
	}

	filter = m_filter_head;
	while (filter) {
		next_filter = (filter_list_t*)filter->next();

		filter->unlink();
		filter->filter->destroy(filter->filter);
		delete filter;

		filter = next_filter;
	}

	if (m_name) {
		free(m_name);
		m_name = NULL;
	}
}

cfilter_module *cdata_stream::search_filter(const char *name)
{
	filter_list_t *iterator;

	iterator = m_filter_head;
	while (iterator) {

		if (!strcmp(name, iterator->filter->name())) {
			break;
		}

		iterator = (filter_list_t*)iterator->next();
	}

	return iterator ? iterator->filter : NULL ;
}

cprocessor_module *cdata_stream::search_processor(const char *name)
{
	processor_list_t *iterator;

	iterator = m_processor_head;

	while (iterator) {
		if (!strcmp(name, iterator->processor->name())) {
			break;
		}

		iterator = (processor_list_t*)iterator->next();
	}

	return iterator ? iterator->processor : NULL ;
}

cdata_stream::processor_list_t *cdata_stream::composite_processor(char *value)
{
	cprocessor_module *processor = NULL;
	char *save_ptr = NULL;
	processor_list_t *item;
	cmodule *module;
	char *token;
	int rtrim_num;
	char proc_name[PROCESSOR_NAME_LENGTH] = {0,};

	//! Name
	token = strtok_r(value, STR_TOKEN_DELIMETER, &save_ptr);
	if (!token) {
		return NULL;
	}
	token = ctrim::ltrim(token);
	rtrim_num = ctrim::rtrim(token);
	processor = (cprocessor_module*)cmodule::search_module(token);
	if (!processor) {
		return NULL;
	}
	//! Get a new name
	token = strtok_r(NULL, STR_TOKEN_DELIMETER, &save_ptr);
	if (!token) {
		return NULL;
	}
	token = ctrim::ltrim(token);
	rtrim_num = ctrim::rtrim(token);
	processor = processor->create_new();
	if (!processor) {
		return NULL;
	}
	DBG("composite_processor : new name : %s\n",token);

	if (processor->update_name(token) == false) {
		ERR("Failed to update name\n");
		//! NOTE: This has no problem :)
		processor->destroy(processor);
		return NULL;
	}

	snprintf(proc_name, PROCESSOR_NAME_LENGTH, "%s", token);
	while((token = strtok_r(NULL, STR_TOKEN_DELIMETER, &save_ptr)) != NULL) {

		token = ctrim::ltrim(token);
		rtrim_num = ctrim::rtrim(token);		

		DBG("Searching module : %s\n", token);

		module = cmodule::search_module(token);
		if (module) {
			if (module->type() == csensor_module::SF_PLUGIN_SENSOR) {
				if (processor->add_input((csensor_module*)module) == false) {
					ERR("Failed to add an input\n");
					continue;
				}
				DBG("Successfully added\n");
			} else if (module->type() == cprocessor_module::SF_PLUGIN_PROCESSOR) {
				DBG("call add_input for processor");
				if (processor->add_input((cprocessor_module*)module) == false) {
					ERR("Failed to add an input\n");
					continue;
				}
				DBG("Successfully added\n");
			} else {
				ERR("Invalid type\n");
			}
		} else {
			ERR("Failed to add %s input\n", token);
			if (!cmodule::unregister_module(proc_name)) {
				ERR("Failed to unregister_module\n");
			}
			return NULL;
		}
	}

	try {
		item = new processor_list_t;
	} catch (...) {
		processor->destroy(processor);
		return NULL;
	}

	item->processor = processor;

	return item;
}

cdata_stream::filter_list_t *cdata_stream::composite_filter(char *value, int multi_check)
{
	cfilter_module *filter = NULL;
	cmodule *module;
	char *token;
	char *save_ptr = NULL;
	int rtrim_num;

	//! Name
	token = strtok_r(value, STR_TOKEN_DELIMETER, &save_ptr);
	if (!token) {
		return NULL;
	}

	token = ctrim::ltrim(token);
	rtrim_num = ctrim::rtrim(token);
	
	DBG("Given module name : %s\n", token);
	filter = (cfilter_module*)cmodule::search_module(token);
	if (!filter) {
		return NULL;
	}

	//! Get a new name
	token = strtok_r(NULL, STR_TOKEN_DELIMETER, &save_ptr);
	if (!token) {
		ERR("No name\n");
		return NULL;
	}

	token = ctrim::ltrim(token);
	rtrim_num = ctrim::rtrim(token);
	
	DBG("Created module name %s\n", token);
	filter = filter->create_new();
	if (!filter) {
		ERR("Failed to create new filter\n");
		return NULL;
	}

	if (filter->update_name(token) == false) {
		ERR("Failed to update name %s\n", token);
		filter->destroy(filter);
		return NULL;
	}

	while((token = strtok_r(NULL, STR_TOKEN_DELIMETER, &save_ptr)) != NULL) {

		token = ctrim::ltrim(token);
		rtrim_num = ctrim::rtrim(token);			

		DBG("Search module : %s\n", token);
		module = cmodule::search_module(token);
		if (!module) {
			ERR("There is no %s sensor\n", token);
			continue;
		}

		if (module->type() != csensor_module::SF_PLUGIN_SENSOR) {
			ERR("Type is not matched\n");			
			continue;
		}

		if (filter->add_input((csensor_module*)module) == false) {
			ERR("Failed to set input sensor_module(%s)\n",module->name());
		} else {
			DBG("added OK, sensor_module : %s \n",module->name());
			if ( multi_check != 1 ) break;
		}
	}

	filter_list_t *item;
	try {
		item = new filter_list_t;
	} catch (...) {
		filter->destroy(filter);
		return NULL;
	}

	item->filter = filter;
	return item;
}

bool cdata_stream::create(const char *conf)
{
	void *handle;
	char *name;
	char *value;
	processor_list_t *processor;
	filter_list_t *filter;
	int idx;
	int multi_stream_state;

	if (m_catalog.load(conf) == false) {
		if ( m_catalog.is_loaded() ) {
			DBG("The catalog is already loaded , so first unload it");
			m_catalog.unload();
		}	
		ERR("Failed to load a configuration file\n");
		return false;
	}

	handle = m_catalog.iterate_init();
	while (handle) {
		name = m_catalog.iterate_get_name(handle);
		handle = m_catalog.iterate_next(handle);
		if (!name) 
			continue;

		value = m_catalog.value(name, (char*)STR_DISABLE);
		if (value && !strcasecmp(value, (char*)STR_YES)) {
			ERR("%s data stream is disabled\n", name);
			continue;
		}
		cdata_stream *data_stream;

		try {
			data_stream = new cdata_stream;
		} catch (...) {
			ERR("No memory");
			continue;
		}

		if (!data_stream->update_name(name)) {
			ERR("Failed to update data_stream name\n");
		}

		value = m_catalog.value(name, (char*)STR_ID);
		if (value) {
			if (!data_stream->update_id(atoi(value))) {
				ERR("Failed to update the id of data_stream\n");
			}
		}

		value = m_catalog.value(name, (char*)STR_MULTI_STREAM);		
		if (value && !strcasecmp(value, (char*)STR_DISABLE)) {
			DBG("Disable multi-data_stream composite\n");
			multi_stream_state = 0;
		} else {
			DBG("Enable multi-data_stream composite\n");
			multi_stream_state = 1;
		}

		idx = 0;
		while ((value = m_catalog.value(name, (char*)STR_FILTER_INPUT, idx))) {
			filter = data_stream->composite_filter(value, multi_stream_state);
			if (filter) {
				if (!data_stream->m_filter_head && !data_stream->m_filter_tail) {
					data_stream->m_filter_head = data_stream->m_filter_tail = filter;
				} else {
					filter->link(clist::AFTER, data_stream->m_filter_tail);
					data_stream->m_filter_tail = filter;
				}
				if ( multi_stream_state != 1 ) break;
			}
			idx ++;
		}

		idx = 0;
		while ((value = m_catalog.value(name, (char*)STR_PROCESSOR_INPUT, idx))) {
			processor = data_stream->composite_processor(value);
			if (processor) {
				if (!data_stream->m_processor_head && !data_stream->m_processor_tail) {
					data_stream->m_processor_head = data_stream->m_processor_tail = processor;
				} else {
					processor->link(clist::AFTER, data_stream->m_processor_tail);
					data_stream->m_processor_tail = processor;
				}
				if ( multi_stream_state != 1 ) break;
			}
			idx ++;
		}
		if ( !data_stream->m_processor_head ) {
			INFO("There is no available processor or filter in the pool for data_stream :%s",name);
			delete data_stream;
		} else {
			DBG("Construct ok data_stream : %s",name);
			data_stream->link(clist::AFTER, m_tail);
			m_tail = data_stream;
		}
	}

	DBG("Finished registeration , unload data_stream.conf ");
	m_catalog.unload();

	return true;
}

void cdata_stream::destroy(void)
{
	if (m_catalog.is_loaded()) {
		m_catalog.unload();
	}
}

char *cdata_stream::name(void)
{
	return m_name;
}

int cdata_stream::id(void)
{
	return m_id;
}

bool cdata_stream::update_name(const char *name)
{
	char *new_name;

	new_name = strdup(name);
	if (!new_name) {
		ERR("No memory\n");
		return false;
	}

	if (m_name) free(m_name);
	m_name = new_name;
	return true;
}

bool cdata_stream::update_id(int id)
{
	m_id = id;
	return true;
}

cprocessor_module *cdata_stream::get_processor(void)
{
	return m_processor_head->processor;
}

cfilter_module *cdata_stream::select_filter(void)
{
	cfilter_module *filter;
	
	if (!m_filter_head) {
		return NULL;
	}

	filter = m_filter_head->filter;
	return filter;
}

cdata_stream *cdata_stream::search_stream(const char *name)
{
	if ( !name ) {
		ERR("Error name ptr check (%s)", __FUNCTION__);
		return NULL;
	}
	cdata_stream *item;
	item = cdata_stream::m_head;
	while (item) {
		if ( item->name() && !strcmp(name, item->name()) ) {
			break;
		}
		else if ( item == item->m_tail ) {
			item = NULL;
			break;
		}
			
		item = (cdata_stream*)item->next();		
	}

	return item;
}

//! End of a file
