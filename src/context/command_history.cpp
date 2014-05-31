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
#include <algorithm>
#include <common.h>
#include <command_history.h>

const char command_history::INST_LIB_ADD = -79;
const char command_history::INST_LIB_REMOVE = -78;
const char command_history::INST_VOICE_ADD = -31;
const char command_history::INST_VOICE_REMOVE = -30;
const char command_history::INST_SET_PROPERTY = -63;

command_history::command_history()
{

}

command_history::~command_history()
{

}


void command_history::input_sensorhub_data(const char* data, int data_len)
{
	const static int MIN_COMMAND_LENGTH = 2;

	if (data_len < MIN_COMMAND_LENGTH)
		return;

	const char inst_type = data[0];

	switch (inst_type) {
	case INST_LIB_ADD:
	case INST_VOICE_ADD:
		register_command(data, data_len);
		break;
	case INST_LIB_REMOVE:
	case INST_VOICE_REMOVE:
		unregister_command(data, data_len);
		break;
	case INST_SET_PROPERTY:
		update_pedo_command(data, data_len);
		break;
	default:
		return;
		break;
	}

	return;
}

bool command_history::get_registered_commands(vector<command>& reg_commands)
{
	command_category::iterator it_category;

	it_category = m_history.begin();

	while (it_category != m_history.end()) {
		command_set::iterator it_command_set;

		it_command_set = it_category->second.begin();

		while (it_command_set != it_category->second.end()) {
			reg_commands.push_back(it_command_set->second);
			++it_command_set;
		}

		++it_category;
	}

	return !reg_commands.empty();

}

bool command_history::is_pedo_registered(void)
{
	return find_command(INST_LIB_ADD, LIB_DATA_TYPE_PEDOMETER);
}

bool command_history::find_command(const char category, const char instruction)
{
	return (m_history[category].find(instruction) != m_history[category].end());
}

void command_history::clear(void)
{
	m_history.clear();
}

void command_history::register_command(const char* cmd, int cmd_len)
{
	m_history[cmd[0]][cmd[1]].assign(cmd, cmd + cmd_len);
}
void command_history::unregister_command(const char* cmd, int cmd_len)
{

/*
 * Currently, the following rule is valid.
 * category_key == reg_command[0] == unreg_command[0] -1
 */
	const char category_key = cmd[0] - 1;
	const char command_key = cmd[1];

	m_history[category_key].erase(command_key);
}
void command_history::update_pedo_command(const char* cmd, int cmd_len)
{
	const static int PROPERTY_COMMAND_LENGTH = 3;

	if (cmd_len != PROPERTY_COMMAND_LENGTH) {
		ERR("propery command lengh(%d) is wrong", cmd_len);
		return;
	}

	if (m_history[INST_LIB_ADD].find(LIB_DATA_TYPE_PEDOMETER) == m_history[INST_LIB_ADD].end()) {
		ERR("Failed to update command pedometer, not registered!");
		return;
	}

	const char property_type = cmd[1];
	const char data = cmd[2];

	switch (property_type) {
	case PEDO_PROPERTY_HEIGHT:
		m_history[INST_LIB_ADD][LIB_DATA_TYPE_PEDOMETER][2] = data;
		break;
	case PEDO_PROPERTY_WEIGHT:
		m_history[INST_LIB_ADD][LIB_DATA_TYPE_PEDOMETER][3] = data;
		break;
	case PEDO_PROPERTY_GENDER:
		m_history[INST_LIB_ADD][LIB_DATA_TYPE_PEDOMETER][4] = data;
		break;
	default:
		break;
	}
}
