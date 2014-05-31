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

#if !defined(_CPACKET_CLASS_H_)
#define _CPACKET_CLASS_H_

typedef struct packet_header {
	int cmd;
	int size;
	char data[0];
} packet_header;

class cpacket
{
public:
	cpacket(int size);
	cpacket(void *data);
	virtual ~cpacket();

	void set_cmd(int cmd);
	int cmd(void);

	void *data(void);
	void *packet(void);

	int size(void);
	int payload_size(void);
	void set_payload_size(int size);

	static int header_size(void);

private:
	enum {
		NEW	= 0x01,
		SET	= 0x02,
	};

	packet_header *m_packet;

	int m_create;
};

#endif
// End of a file
