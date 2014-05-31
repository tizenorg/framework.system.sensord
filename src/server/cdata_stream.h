/*
 * sensor-framework
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

#ifndef CDATA_STREAM_H_
#define CDATA_STREAM_H_

class cdata_stream : public clist
{
public:
	enum value_type_t {
		SENSOR	= 0x01,
		FILTER	= 0x02,
		PROCESSOR = 0x03,
	};

	static const int SF_DATA_STREAM = ctype::UNKNOWN + 40;

	cdata_stream();
	virtual ~cdata_stream();

	static bool create(const char *conf);
	static void destroy(void);

	char *name(void);
	int id(void);

	bool update_name(const char *name);
	bool update_id(int id);

	static cdata_stream *search_stream(const char *name);
	cprocessor_module *get_processor(void);

private:
	struct filter_list_t : public clist {
		cfilter_module *filter;
	};

	struct processor_list_t : public clist {
		cprocessor_module *processor;
	};


	cfilter_module *select_filter(void);

	processor_list_t *composite_processor(char *value);
	filter_list_t *composite_filter(char *value, int multi_chek);

	cfilter_module *search_filter(const char *name);
	cprocessor_module *search_processor(const char *name);

	filter_list_t *m_filter_head;
	filter_list_t *m_filter_tail;

	processor_list_t *m_processor_head;
	processor_list_t *m_processor_tail;

	char *m_name;
	int m_id;

	int m_client;

	static cdata_stream *m_head;
	static cdata_stream *m_tail;
	static ccatalog m_catalog;

	pthread_mutex_t mutex_lock;
};

#endif
//! End of a file
