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

#if !defined(_CLIST_CLASS_H_)
#define _CLIST_CLASS_H_

class clist : public ctype
{
public:
	enum link_t {
		BEFORE	= 0x01,
		AFTER	= 0x02,
		REPLACE	= 0x03,
	};

	clist();
	virtual ~clist();

	virtual clist *next(void);
	virtual clist *prev(void);
	virtual bool link(link_t type, clist *ptr);
	virtual void unlink(void);
	virtual bool add(link_t type, clist *ptr);

	virtual void set_next(clist *item);
	virtual void set_prev(clist *item);

private:
	clist *m_next;
	clist *m_prev;
};

#endif
//! End of a file
