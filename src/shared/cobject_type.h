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

#if !defined(_COBJECT_TYPE_CLASS_)
#define _COBJECT_TYPE_CLASS_

class ctype
{
public:
	//! Every other object can be extend it own value from this.
	//! and developer has to keep its identity.
	//! so if you want use this value, you have to keep your eyes on
	//! its track.
	static const int UNKNOWN= 0xFF;

	ctype(int type = UNKNOWN);
	virtual ~ctype();

	virtual void set_type(int type);
	virtual int type(void);

private:

	int m_type;
};

#endif
//! End of a file
