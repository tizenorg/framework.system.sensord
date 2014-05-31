/*
 * sf-plugin-common
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

#if !defined(_COMMAND_HISTORY_CLASS_H_)
#define _COMMAND_HISTORY_CLASS_H_

#include <map>
#include <vector>

using std::map;
using std::vector;

typedef vector<char> command;
typedef map<char, command> command_set;
typedef map<char, command_set> command_category;

/*
 *
 * In a command,
 * the first value is used as category key and
 * the second value as instruction key.
 * <category key, instrucion key> -> command
 * For example, Registering wrist up -79, 19, 0, 0 will be stored as
 * <-79, <19, <-79, 19, 0, 0> > >
 */
class command_history {
public:
	command_history();
	~command_history();

	void input_sensorhub_data(const char* data, int data_len);
	bool get_registered_commands(vector<command>& reg_commands);
	bool is_pedo_registered(void);
	void clear(void);
private:
	static const char INST_LIB_ADD;
	static const char INST_LIB_REMOVE;
	static const char INST_VOICE_ADD;
	static const char INST_VOICE_REMOVE;
	static const char INST_SET_PROPERTY;

	enum lib_data_type_t {
		LIB_DATA_TYPE_PEDOMETER	= 3,
	};

	enum pedo_property_t {
		PEDO_PROPERTY_HEIGHT	= 18,
		PEDO_PROPERTY_WEIGHT	= 19,
		PEDO_PROPERTY_GENDER	= 20,
	};

	void register_command(const char* cmd, int cmd_len);
	void unregister_command(const char* cmd, int data_len);
	void update_pedo_command(const char* cmd, int cmd_len);
	bool find_command(const char category, const char instruction);

	command_category m_history;
};

#endif

