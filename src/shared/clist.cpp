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
#include <cobject_type.h>
#include <clist.h>
#include "common.h"

clist::clist()
: m_next(NULL)
, m_prev(NULL)
{
}

clist::~clist()
{
}

clist *clist::next(void)
{
	return m_next;
}

clist *clist::prev(void)
{
	return m_prev;
}

bool clist::add(link_t type, clist *ptr)
{
	switch (type) {
	case BEFORE:
		if (prev()) {
			prev()->set_next(ptr);
		}
		ptr->set_prev(prev());

		set_prev(ptr);
		ptr->set_next(this);
		break;
	case AFTER:
		if (next()) {
			next()->set_prev(ptr);
		}
		ptr->set_next(next());

		set_next(ptr);
		ptr->set_prev(this);
		break;
	default:
		return false;
	}

	return true;
}

bool clist::link(link_t type, clist *ptr)
{
	if (!ptr) {
		ERR("Error invalid input ptr handler");
		return false;
	}

	clist *prev;
	clist *next;

	prev = ptr->m_prev;
	next = ptr->m_next;

	switch (type) {
	case BEFORE:
		if (prev) {
			prev->m_next = this;
			m_prev = prev;
		}

		if (!next) {
			ptr->m_next = m_next;
		}

		ptr->m_prev = this;

		m_next = ptr;
		break;

	case AFTER:
		if (next) {
			next->m_prev = this;
			m_next = next;
		}

		if (!prev) {
			ptr->m_prev = m_prev;
		}

		ptr->m_next = this;
		m_prev = ptr;
		break;

	case REPLACE:
		ptr->m_next = m_next;
		ptr->m_prev = m_prev;

		m_next = next;
		m_prev = prev;
		break;

	default:
		ERR("Invalid type\n");
		return false;
	}

	return true;
}

void clist::set_next(clist *item)
{
	m_next = item;
}

void clist::set_prev(clist *item)
{
	m_prev = item;
}

void clist::unlink(void)
{
	if (m_next) {
		m_next->m_prev = m_prev;
	}

	if (m_prev) {
		m_prev->m_next = m_next;
	}

	m_next = NULL;
	m_prev = NULL;
}

//! End of a file
