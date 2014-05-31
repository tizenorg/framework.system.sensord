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

#if !defined(_CMODULE_CLASS_H_)
#define _CMODULE_CLASS_H_

class cmodule : public clist
{
public:
	cmodule();
	virtual ~cmodule();

	virtual const char *name(void) = 0;
	virtual int version(void) = 0;
	virtual int id(void) = 0;

	virtual bool update_name(char *name) = 0;
	virtual bool update_version(int version) = 0;
	virtual bool update_id(int id) = 0;

	static cmodule *register_module(const char *filename, void *a, void *b);
	static bool unregister_module(const char *name);
	static bool unregister_module(cmodule *module);

	static cmodule *search_module(const char *name);

protected:
	static bool add_to_list(cmodule *module);
	static bool del_from_list(cmodule *module);
	static unsigned long long get_timestamp(void);
	static unsigned long long get_timestamp(timeval *t);

private:
	void *m_handle;
	char *m_filename;

	cmodule *(*m_init)(void *a, void *b);
	void (*m_fini)(cmodule *inst);

	static cmodule *m_head;
	static cmodule *m_tail;

	static cmutex m_lock;
};

extern "C" {
extern cmodule *module_init(void *win, void *egl);
extern void module_exit(cmodule *inst);
}

#endif
//! End of a file
