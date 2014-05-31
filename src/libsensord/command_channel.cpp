/*
 * libslp-sensor
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

#include <command_channel.h>
#include <client_common.h>
#include <sf_common.h>


command_channel::command_channel()
: m_command_sock(NULL)
{

}
command_channel::~command_channel()
{
	if (m_command_sock)
		delete m_command_sock;
}

bool command_channel::command_handler(cpacket *packet, void **return_payload)
{
	if (m_command_sock == NULL) {
		ERR("Command socket is not initialized for client %s", get_client_name());
		return false;
	}

	if (!m_command_sock->send(packet->packet(), packet->size())) {
		ERR("Failed to send command in client %s", get_client_name());
		return false;
	}

	packet_header header;

	if (!m_command_sock->recv(&header, sizeof(header))) {
		ERR("Failed to receive header for reply packet in client %s", get_client_name());
		return false;
	}

	char *buffer = new char[header.size];

	if (!m_command_sock->recv(buffer, header.size)) {
		ERR("Failed to receive reply packet in client %s", get_client_name());
		delete[] buffer;
		return false;
	}

	*return_payload = buffer;

	return true;
}

bool command_channel::create_channel(void)
{
	if (m_command_sock) {
		ERR("m_command_sock is alreay allocated for client %s",	get_client_name());
		return false;
	}

	m_command_sock = new csock((char *)COMMAND_CHANNEL_PATH, 0, false);

	if (m_command_sock->connect_to_server() == false) {
		ERR("Failed to connect to server for client %s", get_client_name());
		delete m_command_sock;
		m_command_sock= NULL;
		return false;
	}

	return true;
}


bool command_channel::cmd_get_id(int &client_id)
{
	cpacket *packet;
	cmd_get_id_t *cmd_get_id;
	cmd_done_t *cmd_done;

	packet = new cpacket(sizeof(cmd_get_id_t));

	packet->set_cmd(CMD_GET_ID);
	packet->set_payload_size(sizeof(cmd_get_id_t));

	cmd_get_id = (cmd_get_id_t *)packet->data();
	cmd_get_id->pid = getpid();

	INFO("%s send cmd_get_id()", get_client_name());

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command", get_client_name());
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("client %s got error[%d] from server", get_client_name(), cmd_done->value);
		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	client_id = cmd_done->client_id;

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}

bool command_channel::cmd_hello(int client_id,  sensor_type sensor)
{
	cpacket *packet;
	cmd_hello_t *cmd_hello;
	cmd_done_t *cmd_done;

	packet = new cpacket(sizeof(cmd_hello_t));

	packet->set_cmd(CMD_HELLO);
	packet->set_payload_size(sizeof(cmd_hello_t));

	cmd_hello = (cmd_hello_t*)packet->data();
	cmd_hello->client_id = client_id;
	cmd_hello->sensor = sensor;

	INFO("%s send cmd_hello(client_id=%d, %s)",
		get_client_name(), client_id, get_sensor_name(sensor));

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command for sensor[%s]",
			get_client_name(), get_sensor_name(sensor));
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("client %s got error[%d] from server with sensor [%s]",
			get_client_name(), cmd_done->value, get_sensor_name(sensor));

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}

bool command_channel::cmd_byebye(int client_id, sensor_type sensor)
{
	cpacket *packet;
	cmd_byebye_t *cmd_byebye;
	cmd_done_t *cmd_done;

	packet = new cpacket(sizeof(cmd_byebye_t));

	packet->set_cmd(CMD_BYEBYE);
	packet->set_payload_size(sizeof(cmd_byebye_t));

	cmd_byebye = (cmd_byebye_t*)packet->data();
	cmd_byebye->client_id = client_id;
	cmd_byebye->sensor = sensor;

	INFO("%s send cmd_byebye(client_id=%d, %s)",
		get_client_name(), client_id, get_sensor_name(sensor));

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command for sensor[%s] with client_id [%d]",
			get_client_name(), get_sensor_name(sensor), client_id);
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("Client %s got error[%d] from server for sensor[%s] with client_id [%d]",
			get_client_name(), cmd_done->value, get_sensor_name(sensor), client_id);

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	if (m_command_sock) {
		delete m_command_sock;
		m_command_sock = NULL;
	}

	return true;
}

bool command_channel::cmd_start(int client_id, sensor_type sensor)
{
	cpacket *packet;
	cmd_start_t *cmd_start;
	cmd_done_t *cmd_done;

	packet = new cpacket(sizeof(cmd_start_t));

	packet->set_cmd(CMD_START);
	packet->set_payload_size(sizeof(cmd_start_t));

	cmd_start = (cmd_start_t*)packet->data();
	cmd_start->client_id = client_id;
	cmd_start->sensor = sensor;

	INFO("%s send cmd_start(client_id=%d, %s)",
		get_client_name(), client_id, get_sensor_name(sensor));

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command for sensor[%s] with client_id [%d]",
			get_client_name(), get_sensor_name(sensor), client_id);
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("Client %s got error[%d] from server for sensor[%s] with client_id [%d]",
			get_client_name(), cmd_done->value, get_sensor_name(sensor), client_id);

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}

bool command_channel::cmd_stop(int client_id, sensor_type sensor)
{
	cpacket *packet;
	cmd_stop_t *cmd_stop;
	cmd_done_t *cmd_done;

	packet = new cpacket(sizeof(cmd_stop_t));

	packet->set_cmd(CMD_STOP);
	packet->set_payload_size(sizeof(cmd_stop_t));

	cmd_stop = (cmd_stop_t*)packet->data();
	cmd_stop->client_id = client_id;

	INFO("%s send cmd_stop(client_id=%d, %s)",
		get_client_name(), client_id, get_sensor_name(sensor));

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command for sensor[%s] with client_id [%d]",
			get_client_name(), get_sensor_name(sensor), client_id);
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("Client %s got error[%d] from server for sensor[%s] with client_id [%d]",
			get_client_name(), cmd_done->value, get_sensor_name(sensor), client_id);

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}

bool command_channel::cmd_set_option(int client_id, sensor_type sensor, int option)
{
	cpacket *packet;
	cmd_set_option_t *cmd_set_option;
	cmd_done_t *cmd_done;

	packet = new cpacket(sizeof(cmd_set_option_t));

	packet->set_cmd(CMD_SET_OPTION);
	packet->set_payload_size(sizeof(cmd_set_option_t));

	cmd_set_option = (cmd_set_option_t*)packet->data();
	cmd_set_option->option = option;
	cmd_set_option->client_id = client_id;
	cmd_set_option->sensor = sensor;

	INFO("%s send cmd_set_option(client_id=%d, %s, option=%d)",
		get_client_name(), client_id, get_sensor_name(sensor), option);

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command for sensor[%s] with client_id [%d], option[%]",
			get_client_name(), get_sensor_name(sensor), client_id, option);
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("Client %s got error[%d] from server for sensor[%s] with client_id [%d], option[%]",
			get_client_name(), cmd_done->value, get_sensor_name(sensor), client_id, option);

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}

bool command_channel::cmd_register_event(int client_id, unsigned int event_type)
{
	cpacket *packet;
	cmd_reg_t *cmd_reg;
	cmd_done_t *cmd_done;

	packet = new cpacket(sizeof(cmd_reg_t));

	packet->set_cmd(CMD_REG);
	packet->set_payload_size(sizeof(cmd_reg_t));

	cmd_reg = (cmd_reg_t*)packet->data();
	cmd_reg->client_id = client_id;
	cmd_reg->event_type = event_type;

	INFO("%s send cmd_register_event(client_id=%d, %s)",
		get_client_name(), client_id, get_event_name(event_type));

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command with client_id [%d], event_type[%s]",
			get_client_name(), client_id, get_event_name(event_type));
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("Client %s got error[%d] from server with client_id [%d], event_type[%s]",
			get_client_name(), cmd_done->value, client_id, get_event_name(event_type));

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}


bool command_channel::cmd_register_events(int client_id, event_type_vector &event_vec)
{
	event_type_vector::iterator it_event;

	it_event = event_vec.begin();

	while (it_event != event_vec.end()) {
		if(!cmd_register_event(client_id, *it_event))
			return false;

		++it_event;
	}

	return true;
}

bool command_channel::cmd_unregister_event(int client_id, unsigned int event_type)
{
	cpacket *packet;
	cmd_unreg_t *cmd_unreg;
	cmd_done_t *cmd_done;

	packet = new cpacket(sizeof(cmd_unreg_t));

	packet->set_cmd(CMD_UNREG);
	packet->set_payload_size(sizeof(cmd_unreg_t));

	cmd_unreg = (cmd_unreg_t*)packet->data();
	cmd_unreg->client_id = client_id;
	cmd_unreg->event_type = event_type;

	INFO("%s send cmd_unregister_event(client_id=%d, %s)",
		get_client_name(), client_id, get_event_name(event_type));

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command with client_id [%d], event_type[%s]",
			get_client_name(), client_id, get_event_name(event_type));
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("Client %s got error[%d] from server with client_id [%d], event_type[%s]",
			get_client_name(), cmd_done->value, client_id, get_event_name(event_type));

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}


bool command_channel::cmd_unregister_events(int client_id, event_type_vector &event_vec)
{
	event_type_vector::iterator it_event;

	it_event = event_vec.begin();

	while (it_event != event_vec.end()) {
		if(!cmd_unregister_event(client_id, *it_event))
			return false;

		++it_event;
	}

	return true;
}


bool command_channel::cmd_check_event(int client_id, unsigned int event_type)
{
	cpacket *packet;
	cmd_check_event_t *cmd_check_event;
	cmd_done_t *cmd_done;

	packet = new cpacket(sizeof(cmd_check_event_t));

	packet->set_cmd(CMD_CHECK_EVENT);
	packet->set_payload_size(sizeof(cmd_check_event_t));

	cmd_check_event = (cmd_check_event_t*)packet->data();
	cmd_check_event->client_id = client_id;
	cmd_check_event->event_type = event_type;

	INFO("%s send cmd_check_event(client_id=%d, %s)",
		get_client_name(), client_id, get_event_name(event_type));

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command with client_id [%d], event_type[%s]",
			get_client_name(), client_id, get_event_name(event_type));
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}

bool command_channel::cmd_set_interval(int client_id, sensor_type sensor, unsigned int interval)
{
	cpacket *packet;
	cmd_set_interval_t *cmd_set_interval;
	cmd_done_t *cmd_done;

	packet = new cpacket(sizeof(cmd_set_interval_t));

	packet->set_cmd(CMD_SET_INTERVAL);
	packet->set_payload_size(sizeof(cmd_set_interval_t));

	cmd_set_interval = (cmd_set_interval_t*)packet->data();
	cmd_set_interval->client_id = client_id;
	cmd_set_interval->interval = interval;
	cmd_set_interval->sensor = sensor;

	INFO("%s send cmd_set_interval(client_id=%d, %s, interval=%d)",
		get_client_name(), client_id, get_sensor_name(sensor), interval);

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("%s failed to send/receive command for sensor[%s] with client_id [%d], interval[%d]",
			get_client_name(), get_sensor_name(sensor), client_id, interval);
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("%s got error[%d] from server for sensor[%s] with client_id [%d], interval[%d]",
			get_client_name(), cmd_done->value, get_sensor_name(sensor), client_id, interval);

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}

bool command_channel::cmd_unset_interval(int client_id, sensor_type sensor)
{
	cpacket *packet;
	cmd_unset_interval_t *cmd_unset_interval;
	cmd_done_t *cmd_done;

	packet = new cpacket(sizeof(cmd_unset_interval_t));

	packet->set_cmd(CMD_UNSET_INTERVAL);
	packet->set_payload_size(sizeof(cmd_unset_interval_t));

	cmd_unset_interval = (cmd_unset_interval_t*)packet->data();
	cmd_unset_interval->client_id = client_id;
	cmd_unset_interval->sensor = sensor;

	INFO("%s send cmd_unset_interval(client_id=%d, %s)",
		get_client_name(), client_id, get_sensor_name(sensor));

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command for sensor[%s] with client_id [%d]",
			get_client_name(), get_sensor_name(sensor), client_id);
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("Client %s got error[%d] from server for sensor[%s] with client_id [%d]",
			get_client_name(), cmd_done->value, get_sensor_name(sensor), client_id);

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}

static bool is_sensor_property(unsigned int data_id)
{
	return ((data_id & 0xFFFF) == 1);
}

bool command_channel::cmd_get_property(int client_id, sensor_type sensor, unsigned int data_id, void *property_data)
{
	cpacket *packet;
	cmd_get_property_t *cmd_get_property;
	cmd_return_property_t *cmd_return_property;

	packet = new cpacket(sizeof(cmd_get_property_t));

	packet->set_cmd(CMD_GET_PROPERTY);
	packet->set_payload_size(sizeof(cmd_get_property_t));

	cmd_get_property = (cmd_get_property_t*)packet->data();
	cmd_get_property->client_id = client_id;
	cmd_get_property->property_level = data_id;

	INFO("%s send cmd_get_property(client_id=%d, %s, %s)",
		get_client_name(), client_id, get_sensor_name(sensor), get_data_name(data_id));

	if (!command_handler(packet, (void **)&cmd_return_property)) {
		ERR("Client %s failed to send/receive command for sensor[%s] with client_id [%d], data_id[%s]",
			get_client_name(), get_sensor_name(sensor), client_id, get_data_name(data_id));
		delete packet;
		return false;
	}

	if (cmd_return_property->state < 0) {
		ERR("Client %s got error[%d] from server for sensor[%s] with client_id [%d], data_id[%s]",
			get_client_name(), cmd_return_property->state, get_sensor_name(sensor), client_id, get_data_name(data_id));


		delete[] (char *)cmd_return_property;
		delete packet;
		return false;
	}

	base_property_struct *base_property;
	base_property = &cmd_return_property->base_property;

	if (is_sensor_property(data_id)) {
		sensor_properties_t *sensor_property;
		sensor_property = (sensor_properties_t *)property_data;
		sensor_property->sensor_unit_idx = base_property->sensor_unit_idx;
		sensor_property->sensor_min_range = base_property->sensor_min_range;
		sensor_property->sensor_max_range = base_property->sensor_max_range;
		sensor_property->sensor_resolution = base_property->sensor_resolution;
		strncpy(sensor_property->sensor_name, base_property->sensor_name, strlen(base_property->sensor_name));
		strncpy(sensor_property->sensor_vendor, base_property->sensor_vendor, strlen(base_property->sensor_vendor));
	} else {
		sensor_data_properties_t *data_property;
		data_property = (sensor_data_properties_t *)property_data;
		data_property->sensor_unit_idx = base_property->sensor_unit_idx ;
		data_property->sensor_min_range= base_property->sensor_min_range;
		data_property->sensor_max_range= base_property->sensor_max_range;
		data_property->sensor_resolution = base_property->sensor_resolution;
	}

	delete[] (char *)cmd_return_property;
	delete packet;

	return true;
}

bool command_channel::cmd_set_value(int client_id, sensor_type sensor, unsigned int property, long value)
{
	cpacket *packet;
	cmd_set_value_t *cmd_set_value;
	cmd_done_t *cmd_done;

	packet = new cpacket(sizeof(cmd_set_value_t));

	packet->set_cmd(CMD_SET_VALUE);
	packet->set_payload_size(sizeof(cmd_set_value_t));

	cmd_set_value = (cmd_set_value_t*)packet->data();
	cmd_set_value->client_id = client_id;
	cmd_set_value->sensor = sensor;
	cmd_set_value->property = property;
	cmd_set_value->value = value;


	INFO("%s send cmd_set_value(client_id=%d, %s, 0x%x, %ld)",
		get_client_name(), client_id, get_sensor_name(sensor), property, value);

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("Client %s failed to send/receive command for sensor[%s] with client_id [%d], property[0x%x], value[%d]",
			get_client_name(), get_sensor_name(sensor), client_id, property, value);
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("Client %s got error[%d] from server for sensor[%s] with property[0x%x], value[%d]",
			get_client_name(), cmd_done->value, get_sensor_name(sensor), property, value);

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;
}

bool command_channel::cmd_get_struct(int client_id, unsigned int data_id, sensor_data_t* sensor_data)
{
	cpacket *packet;
	cmd_get_data_t *cmd_get_data;
	cmd_get_struct_t *cmd_get_struct;

	packet = new cpacket(sizeof(cmd_get_struct_t));

	packet->set_cmd(CMD_GET_STRUCT);
	packet->set_payload_size(sizeof(cmd_get_data_t));

	cmd_get_data = (cmd_get_data_t*)packet->data();
	cmd_get_data->client_id = client_id;
	cmd_get_data->data_id = data_id;

	if (!command_handler(packet, (void **)&cmd_get_struct)) {
		ERR("Client %s failed to send/receive command with client_id [%d], data_id[%s]",
			get_client_name(), client_id, get_data_name(data_id));
		delete packet;
		return false;
	}

	if (cmd_get_struct->state < 0 ) {
		ERR("Client %s got error[%d] from server with client_id [%d], data_id[%s]",
			get_client_name(), cmd_get_struct->state, client_id, get_data_name(data_id));
		sensor_data->data_accuracy = SENSOR_ACCURACY_UNDEFINED;
		sensor_data->data_unit_idx = SENSOR_UNDEFINED_UNIT;
		sensor_data->time_stamp = 0;
		sensor_data->values_num = 0;
		delete[] (char *)cmd_get_struct;
		delete packet;
		return false;
	}

	base_data_struct *base_data;
	base_data = &cmd_get_struct->base_data;

	sensor_data->time_stamp = base_data->time_stamp;
	sensor_data->data_accuracy = base_data->data_accuracy;
	sensor_data->data_unit_idx = base_data->data_unit_idx;
	sensor_data->values_num = base_data->values_num;

	memcpy(sensor_data->values, base_data->values,
		sizeof(sensor_data->values[0]) * base_data->values_num);

	delete[] (char *)cmd_get_struct;
	delete packet;

	return true;
}

bool command_channel::cmd_send_sensorhub_data(int client_id, int data_len, const char* buffer)
{
	cpacket *packet;
	cmd_send_sensorhub_data_t *cmd_send_sensorhub_data;
	cmd_done_t *cmd_done;

	packet = new cpacket(sizeof(cmd_send_sensorhub_data_t) + data_len);
	packet->set_cmd(CMD_SEND_SENSORHUB_DATA);

	cmd_send_sensorhub_data = (cmd_send_sensorhub_data_t*)packet->data();
	cmd_send_sensorhub_data->client_id = client_id;
	cmd_send_sensorhub_data->data_len = data_len;
	memcpy(cmd_send_sensorhub_data->data, buffer, data_len);


	INFO("%s send cmd_send_sensorhub_data(client_id=%d, data_len = %d, buffer = 0x%x)",
		get_client_name(), client_id, data_len, buffer);

	if (!command_handler(packet, (void **)&cmd_done)) {
		ERR("%s failed to send/receive command with client_id [%d]",
			get_client_name(), client_id);
		delete packet;
		return false;
	}

	if (cmd_done->value < 0) {
		ERR("%s got error[%d] from server with client_id [%d]",
			get_client_name(), cmd_done->value, client_id);

		delete[] (char *)cmd_done;
		delete packet;
		return false;
	}

	delete[] (char *)cmd_done;
	delete packet;

	return true;


}
