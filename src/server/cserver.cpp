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

#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#include <time.h>
#include <common.h>
#include <cobject_type.h>
#include <clist.h>
#include <cmutex.h>
#include <cworker.h>
#include <cipc_worker.h>
#include <csock.h>
#include <cmodule.h>
#include <cpacket.h>
#include <sf_common.h>
#include <ccatalog.h>
#include <csensor_module.h>
#include <cfilter_module.h>
#include <cprocessor_module.h>
#include <cdata_stream.h>
#include <cclient_info_manager.h>
#include <cserver.h>
#include <resource_str.h>
#include <csensor_event_dispatcher.h>
#include <csensor_event_queue.h>
#include <glib.h>
#include <vconf.h>
#include <string>
#include <systemd/sd-daemon.h>

using std::string;

#define POWEROFF_NOTI_NAME "power_off_start"
#define SF_POWEROFF_VCONF "memory/private/sensor/poweroff"

static const int OP_ERROR = -1;
static const int OP_SUCCESS = 0;


static GMainLoop *g_mainloop;
cserver cserver::inst;

const char* get_datastream_name(sensor_id_t id)
{
	switch (id) {
	case ID_ACCEL:
		return "accel_datastream";
	case ID_GEOMAG:
		return "geomag_datastream";
	case ID_LUMINANT:
		return "lumin_datastream";
	case ID_PROXI:
		return "proxi_datastream";
	case ID_GYROSCOPE:
		return "gyro_datastream";
	case ID_PRESSURE:
		return "barometer_datastream";
	case ID_MOTION_ENGINE:
		return "motion_datastream";
	case ID_FUSION:
		return "fusion_datastream";
	case ID_PEDO:
		return "pedo_datastream";
	case ID_CONTEXT:
		return "context_datastream";
	case ID_FLAT:
		return "flat_datastream";
	case ID_BIO:
		return "bio_datastream";
	case ID_BIO_HRM:
		return "bio_hrm_datastream";
	default:
		return 0;
	}
}

cserver::cserver()
{
	m_cmd_handler[CMD_GET_ID]				= cmd_get_id;
	m_cmd_handler[CMD_HELLO]				= cmd_hello;
	m_cmd_handler[CMD_BYEBYE]				= cmd_byebye;
	m_cmd_handler[CMD_START]				= cmd_start;
	m_cmd_handler[CMD_STOP]					= cmd_stop;
	m_cmd_handler[CMD_REG]					= cmd_register_event;
	m_cmd_handler[CMD_UNREG]				= cmd_unregister_event;
	m_cmd_handler[CMD_CHECK_EVENT]			= cmd_check_event;
	m_cmd_handler[CMD_SET_OPTION]			= cmd_set_option;
	m_cmd_handler[CMD_SET_INTERVAL]			= cmd_set_interval;
	m_cmd_handler[CMD_UNSET_INTERVAL]		= cmd_unset_interval;
	m_cmd_handler[CMD_SET_VALUE]			= cmd_set_value;
	m_cmd_handler[CMD_GET_PROPERTY]			= cmd_get_property;
	m_cmd_handler[CMD_GET_STRUCT]			= cmd_get_struct;
	m_cmd_handler[CMD_SEND_SENSORHUB_DATA]	= cmd_send_sensorhub_data;
}

static bool send_cmd_done(csock* command_sock, int client_id, long value)
{
	cpacket* ret_packet;
	cmd_done_t *cmd_done;

	ret_packet = new cpacket(sizeof(cmd_done_t));
	ret_packet->set_cmd(CMD_DONE);

	cmd_done = (cmd_done_t*)ret_packet->data();
	cmd_done->client_id = client_id;
	cmd_done->value = value;

	if (!command_sock->send(ret_packet->packet(), ret_packet->size())) {
		ERR("Failed to send a cmd_done to client_id [%d] with value [%ld]", client_id, value);
		delete ret_packet;
		return false;
	}

	delete ret_packet;
	return true;

}


static bool send_cmd_return_property(csock* command_sock, int state, base_property_struct* base_property)
{
	cpacket* ret_packet;
	cmd_return_property_t *cmd_return_property;

	ret_packet = new cpacket(sizeof(cmd_return_property_t));
	ret_packet->set_cmd(CMD_GET_PROPERTY);

	cmd_return_property = (cmd_return_property_t*)ret_packet->data();
	cmd_return_property->state = state;
	memcpy(&cmd_return_property->base_property, base_property , sizeof(base_property_struct));

	if (!command_sock->send(ret_packet->packet(), ret_packet->size())) {
		ERR("Failed to send a cmd_get_property");
		delete ret_packet;
		return false;
	}

	delete ret_packet;
	return true;

}

static bool send_cmd_get_struct(csock* command_sock, int state, base_data_struct *base_data)
{

	cpacket* ret_packet;
	cmd_get_struct_t *cmd_get_struct;

	ret_packet = new cpacket(sizeof(cmd_get_struct_t));
	ret_packet->set_cmd(CMD_GET_STRUCT);

	cmd_get_struct = (cmd_get_struct_t*)ret_packet->data();
	cmd_get_struct->state = state;

	memcpy(&cmd_get_struct->base_data , base_data , sizeof(base_data_struct));

	if (!command_sock->send(ret_packet->packet(), ret_packet->size())) {
		ERR("Failed to send a cmd_get_struct");
		delete ret_packet;
		return false;
	}

	delete ret_packet;
	return true;


}

void *cserver::cmd_get_id(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cmd_get_id_t *payload;

	csock ipc(arg->client_handle);
	ipc.set_on_close(false);
	int client_id;
	long ret_value = OP_SUCCESS;

	DBG("CMD_GET_ID Handler invoked\n");
	payload = (cmd_get_id_t*)queue_item->packet->data();

	client_id = get_client_info_manager().create_client_record();
	get_client_info_manager().set_client_info(client_id, payload->pid);
	ctx->client_id = client_id;

	INFO("New client id [%d] created", client_id);

	if (!send_cmd_done(&ipc, client_id, ret_value))
		ERR("Failed to send cmd_done to a client");

	return (void *)NULL;
}



void *cserver::cmd_hello(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx;
	cmd_hello_t *payload;
	csock ipc(arg->client_handle);
	ipc.set_on_close(false);
	int client_id;
	long ret_value = OP_ERROR;
	const char *datastream_name;

	DBG("CMD_HELLO Handler invoked\n");
	ctx = (client_ctx_t*)arg->client_ctx;
	payload = (cmd_hello_t*)queue_item->packet->data();

	client_id = payload->client_id;

	sensor_id_t id = static_cast<sensor_id_t>(payload->sensor);

	datastream_name = get_datastream_name(id);

	DBG("Hello sensor [0x%x], sensor name [%s], client id [%d]", payload->sensor, datastream_name, client_id);

	ctx->module = (ctype*)cdata_stream::search_stream(datastream_name);

	if (!ctx->module) {
		ERR("There is no proper module found for %s", datastream_name ? datastream_name : "NULL");
		if (!get_client_info_manager().has_sensor_record(client_id))
			get_client_info_manager().remove_client_record(client_id);
	} else {
		ctx->client_state = 0;

		get_client_info_manager().create_sensor_record(client_id, (sensor_type)payload->sensor);
		INFO("New sensor record created for sensor [0x%x], sensor name [%s] on client id [%d]\n", payload->sensor, datastream_name, client_id);
		ret_value = OP_SUCCESS;
	}

	if (!send_cmd_done(&ipc, client_id, ret_value))
		ERR("Failed to send cmd_done to a client");

	return (void *)NULL;
}

void *cserver::cmd_byebye(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx;
	cmd_byebye_t *payload;
	cpacket *packet = queue_item->packet;
	csock ipc(arg->client_handle);
	ipc.set_on_close(false);
	long ret_value;

	ctx = (client_ctx_t*)arg->client_ctx;

	if (arg->worker->state() != cipc_worker::START) {
		ERR("Client disconnected");
		return (void*)NULL;
	}

	DBG("CMD_BYEBYE Handler invoked ctx->client_state = %d\n", ctx->client_state);

	payload = (cmd_byebye_t*)packet->data();

	INFO("BYEBYE for client [%d], sensor [0x%x]\n", payload->client_id, payload->sensor);

	if(!get_client_info_manager().remove_sensor_record(payload->client_id, (const sensor_type) payload->sensor)) {
		ERR("Error removing sensor_record for client [%d]", payload->client_id);
		ret_value = OP_ERROR;
	} else {
		ret_value = OP_SUCCESS;
	}
	
	arg->worker->stop();

	if (!send_cmd_done(&ipc, payload->client_id, ret_value))
		ERR("Failed to send cmd_done to a client");
	
	return (void *)NULL;
}

void *cserver::cmd_start(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cpacket *packet = queue_item->packet;
	csock ipc(arg->client_handle);
	cmd_start_t *payload;
	long value = OP_SUCCESS;

	ipc.set_on_close(false);

	payload = (cmd_start_t*)packet->data();
	
	if (arg->worker->state() != cipc_worker::START) {
		ERR("Client disconnected (%s)\n",__FUNCTION__);
		value = OP_ERROR;
		goto out;
	}

	DBG("CMD_START Handler invoked ctx->client_state = %d\n", ctx->client_state);
	
	DBG("START Sensor [0x%x], called from client [%d]", payload->sensor, payload->client_id);

	if ( !(ctx->module) ) {
		ERR("Error ctx->module ptr check(%s)", __FUNCTION__);
		value = OP_ERROR;
		goto out;
	}
	
	if (ctx->module->type() == cdata_stream::SF_DATA_STREAM) {
		cdata_stream *stream = (cdata_stream*)ctx->module;
		DBG("Invoke Stream start for [%s]\n",stream->name());
		if (ctx->client_state > 0) {
			ERR("This module has already started\n");
		} else {
			if (stream->get_processor()->start()) {
				ctx->client_state++;
/*
 *	Rotation could be changed even LCD is off by pop sync rotation
 *	and a client listening rotation event with always-on option.
 *	To reflect the last rotation state, request it to event dispatcher.
 */
				get_event_dispathcher().request_last_event(payload->client_id,
					(const sensor_type)payload->sensor);
			} else {
				value = OP_ERROR;
			}
		}
	} else {
		ERR("This module has no start operation (%s)\n",__FUNCTION__);
	}

out:
	if (!send_cmd_done(&ipc, payload->client_id, value))
		ERR("Failed to send cmd_done to a client");
	
	return (void*)NULL;
}

void *cserver::cmd_stop(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cpacket *packet = queue_item->packet;
	csock ipc(arg->client_handle);
	ipc.set_on_close(false);
	long ret_val = OP_SUCCESS;

	cmd_stop_t *payload;
		
	if (arg->worker->state() != cipc_worker::START) {
		ERR("Client disconnected (%s)\n",__FUNCTION__);
		return (void*)NULL;
	}

	DBG("CMD_STOP Handler invoked ctx->client_state = %d\n", ctx->client_state);

	INFO("STOP client invoked\n");

	payload = (cmd_stop_t*)packet->data();

	if ( !(ctx->module) ) {
		ERR("Error ctx->module ptr check(%s)", __FUNCTION__);
		ret_val = OP_ERROR;
		goto out;
	}
	
	if (ctx->module->type() == cdata_stream::SF_DATA_STREAM) {
		cdata_stream *stream = (cdata_stream*)ctx->module;
		//! Before stopping stream, we need to wakeup the clients
		DBG("Invoke Stream stop\n");
		if (ctx->client_state < 1) {
			ERR("This module has already stopped\n");
			ret_val = OP_ERROR;
		}
		else {
			DBG("Now stop %s \n", stream->name());
			if (stream->get_processor()->stop() ) {
				ctx->client_state --;
			}
		}
	} else {
		ERR("This module has no stop operation (%s)\n",__FUNCTION__);
		ret_val = OP_ERROR;
	}

out:
	if (!send_cmd_done(&ipc, payload->client_id, ret_val))
		ERR("Failed to send cmd_done to a client");

	return (void*)NULL;
}

void *cserver::cmd_register_event(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cdata_stream *data_stream = (cdata_stream*)ctx->module;

	cmd_reg_t *payload;

	csock ipc(arg->client_handle);
	ipc.set_on_close(false);

	cpacket *packet = queue_item->packet;
	long ret_val = OP_ERROR;

	payload = (cmd_reg_t*)packet->data();

	if (!get_client_info_manager().register_event(payload->client_id, payload->event_type)) {
		INFO("Failed to register event [0x%x] for client [%d] to client info manager",
			payload->event_type, payload->client_id);
		goto out;
	}

	data_stream->get_processor()->add_client(payload->event_type);

	ctx->reg_state = (ctx->reg_state) | (payload->event_type) ;
	ret_val = OP_SUCCESS;
	DBG("Registering Event [0x%x] is done for client [%d]", payload->event_type, payload->client_id);

out:
	if (!send_cmd_done(&ipc, payload->client_id, ret_val))
		ERR("Failed to send cmd_done to a client");

	return (void *)NULL;
}

void *cserver::cmd_unregister_event(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cdata_stream *data_stream = (cdata_stream*)ctx->module;

	cmd_unreg_t *payload;

	csock ipc(arg->client_handle);
	ipc.set_on_close(false);

	cpacket *packet = queue_item->packet;
	long ret_val = OP_ERROR;

	payload = (cmd_unreg_t*)packet->data();

	if (!get_client_info_manager().unregister_event(payload->client_id, payload->event_type)) {
		ERR("Failed to unregister event [0x%x] for client [%d from client info manager",
			payload->event_type, payload->client_id);
		goto out;
	}

	if (!data_stream->get_processor()->delete_client(payload->event_type)) {
		ERR("Failed to unregister event [0x%x] for client [%d]",
			payload->event_type, payload->client_id);
		goto out;
	}

	ctx->reg_state = (ctx->reg_state) ^ (payload->event_type & 0xFFFF);
	ret_val = OP_SUCCESS;
	DBG("Unregistering Event [0x%x] is done for client [%d]",
		payload->event_type, payload->client_id);

out:
	if (!send_cmd_done(&ipc, payload->client_id, ret_val))
		ERR("Failed to send cmd_done to a client");

	return (void *)NULL;
}


void *cserver::cmd_check_event(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cdata_stream *data_stream = (cdata_stream*)ctx->module;

	cmd_check_event_t *payload;

	csock ipc(arg->client_handle);
	ipc.set_on_close(false);

	cpacket *packet = queue_item->packet;
	long ret_val = OP_ERROR;

	payload = (cmd_check_event_t*)packet->data();

	if (data_stream->get_processor()->is_supported(payload->event_type)) {
		ret_val = OP_SUCCESS;
		DBG("Event[0x%x] is supported for client [%d], for sensor [0x%x]", payload->event_type, payload->client_id, (payload->event_type >> 16));
	}

	if (!send_cmd_done(&ipc, payload->client_id, ret_val))
		ERR("Failed to send cmd_done to a client");

	return (void *)NULL;
}


void *cserver::cmd_set_interval(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cdata_stream *data_stream = (cdata_stream*)ctx->module;

	cmd_set_interval_t *payload;

	csock ipc(arg->client_handle);
	ipc.set_on_close(false);

	cpacket *packet = queue_item->packet;
	long ret_val = OP_ERROR;

	payload = (cmd_set_interval_t*)packet->data();


	if (!get_client_info_manager().set_interval(payload->client_id, (sensor_type) payload->sensor, payload->interval)) {
		ERR("Failed to register interval for client [%d], for sensor [0x%x] with interval [%d] to client info manager",
			payload->client_id, payload->sensor, payload->interval);
		goto out;
	}

	if (!data_stream->get_processor()->add_interval(payload->client_id, payload->interval, false)) {
		ERR("Failed to set interval for client [%d], for sensor [0x%x] with interval [%d]",
			payload->client_id, payload->sensor, payload->interval);
		goto out;
	}

	ret_val = OP_SUCCESS;

out:
	if (!send_cmd_done(&ipc, payload->client_id, ret_val))
		ERR("Failed to send cmd_done to a client");

	return (void *)NULL;
}


void *cserver::cmd_unset_interval(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cdata_stream *data_stream = (cdata_stream*)ctx->module;

	cmd_unset_interval_t *payload;

	csock ipc(arg->client_handle);
	ipc.set_on_close(false);

	cpacket *packet = queue_item->packet;
	long ret_val = OP_ERROR;

	payload = (cmd_unset_interval_t*)packet->data();

	if (!get_client_info_manager().set_interval(payload->client_id, (sensor_type) payload->sensor, 0)) {
		ERR("Failed to unregister interval for client [%d], for sensor [0x%x] to client info manager",
			payload->client_id, payload->sensor);
		goto out;
	}

	if (!data_stream->get_processor()->delete_interval(payload->client_id, false)) {
		ERR("Failed to delete interval for client [%d]", payload->client_id);
		goto out;
	}

	ret_val = OP_SUCCESS;

out:
	if (!send_cmd_done(&ipc, payload->client_id, ret_val))
		ERR("Failed to send cmd_done to a client");

	return (void *)NULL;
}



void *cserver::cmd_set_option(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;

	cmd_set_option_t *payload;

	csock ipc(arg->client_handle);
	ipc.set_on_close(false);

	cpacket *packet = queue_item->packet;
	long ret_val = OP_ERROR;

	payload = (cmd_set_option_t*)packet->data();

	if (!get_client_info_manager().set_option(payload->client_id, (sensor_type)payload->sensor, payload->option)) {
		ERR("Failed to register interval for client [%d], for sensor [0x%x] with option [%d] to client info manager",
			payload->client_id, payload->sensor, payload->option);
		goto out;
	}

	ret_val = OP_SUCCESS;
out:
	if (!send_cmd_done(&ipc, payload->client_id, ret_val))
		ERR("Failed to send cmd_done to a client");

	return (void *)NULL;	
}

void *cserver::cmd_set_value(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cmd_set_value_t *payload;

	csock ipc(arg->client_handle);
	ipc.set_on_close(false);
	cpacket *packet = queue_item->packet;
	long ret_val = OP_ERROR;

	if (arg->worker->state() != cipc_worker::START) {
		ERR("Client disconnected (%s)\n",__FUNCTION__);
		return (void*)NULL;
	}

	INFO("CMD_SET_VALUE  Handler invoked\n");

	payload = (cmd_set_value_t*)packet->data();

	if ( !(ctx->module) ) {
		ERR("Error ctx->module ptr check(%s)", __FUNCTION__);
		goto out;
	}

	if (ctx->module->type() == cdata_stream::SF_DATA_STREAM) {
		cdata_stream *data_stream;
		data_stream = (cdata_stream*)ctx->module;
		ret_val = data_stream->get_processor()->set_cmd(payload->sensor, payload->property, payload->value);
	}

out:
	if (!send_cmd_done(&ipc, payload->client_id, ret_val))
		ERR("Failed to send cmd_done to a client");

	return (void *)NULL;
}

void *cserver::cmd_get_property(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cmd_get_property_t *payload;
	csock ipc(arg->client_handle);
	ipc.set_on_close(false);
	cpacket *packet = queue_item->packet;
	int state = OP_ERROR;
	base_property_struct base_property;

	memset(&base_property, 0, sizeof(base_property));

	if (arg->worker->state() != cipc_worker::START) {
		ERR("Client disconnected (%s)\n",__FUNCTION__);
		return (void*)NULL;
	}

	INFO("CMD_GET_PROPERTY Handler invoked\n");

	payload = (cmd_get_property_t*)packet->data();
	
	if ( !(ctx->module) ) {
		ERR("Error ctx->module ptr check(%s)", __FUNCTION__);
		goto out;
	}
			
	if (ctx->module->type() == cdata_stream::SF_DATA_STREAM) {
		cdata_stream *data_stream;
		data_stream = (cdata_stream*)ctx->module;

		state = data_stream->get_processor()->get_property(payload->property_level, (void*)&base_property);
		if (state != 0) {
			ERR("data_stream get_property fail");
			goto out;
		}
	}

out:

	if (!send_cmd_return_property(&ipc, state, &base_property))
		ERR("Failed to send cmd_done to a client");

	return (void *)NULL;
}

void *cserver::cmd_get_struct(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cmd_get_data_t *payload;
	csock ipc(arg->client_handle);
	ipc.set_on_close(false);
	cpacket *packet = queue_item->packet;
	int state = OP_ERROR;

	unsigned int struct_data_id = 0;
	base_data_struct base_data;

	if (arg->worker->state() != cipc_worker::START) {
		ERR("Client disconnected (%s)\n",__FUNCTION__);
		return (void*)NULL;
	}

	INFO("CMD_GET_VALUE Handler invoked\n");

	payload = (cmd_get_data_t*)packet->data();
	struct_data_id = payload->data_id;	

	if ( !(ctx->module) ) {
		ERR("Error ctx->module ptr check(%s)", __FUNCTION__);
		goto out;
	}

	if (ctx->module->type() == cdata_stream::SF_DATA_STREAM) {
		cdata_stream *data_stream;
		data_stream = (cdata_stream*)ctx->module;
		state = data_stream->get_processor()->get_struct_value(struct_data_id, (void*)&base_data);
		if (state != 0) {
			ERR("data_stream cmd_get_struct fail");
			goto out;
		}
	} else {
		ERR("Does not support module type for cmd_get_struct , current module_type : %d\n", ctx->module->type());
	}

out:
	send_cmd_get_struct(&ipc, state, &base_data);

	return (void *)NULL;
}

void *cserver::cmd_send_sensorhub_data(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	csock::thread_arg_t *arg = queue_item->arg;
	client_ctx_t *ctx = (client_ctx_t*)arg->client_ctx;
	cmd_send_sensorhub_data_t *payload;

	csock ipc(arg->client_handle);
	ipc.set_on_close(false);

	cpacket *packet = queue_item->packet;
	long ret_val = OP_ERROR;

	if (arg->worker->state() != cipc_worker::START) {
		ERR("Connection is disconnected");
		return (void*)NULL;
	}

	INFO("CMD_SEND_SENSORHUB_DATA Handler invoked");

	payload = (cmd_send_sensorhub_data_t*)packet->data();

	if (!ctx->module) {
		ERR("ctx->module is null");
		goto out;
	}

	if (ctx->module->type() == cdata_stream::SF_DATA_STREAM) {
		cdata_stream *data_stream;
		data_stream = (cdata_stream*)ctx->module;
		ret_val = data_stream->get_processor()->send_sensorhub_data(payload->data, payload->data_len);
	}

out:
	if (!send_cmd_done(&ipc, payload->client_id, ret_val))
		ERR("Failed to send cmd_done to a client");

	return (void *)NULL;
}

cclient_info_manager& cserver::get_client_info_manager(void)
{
	return cclient_info_manager::get_instance();
}

csensor_event_dispatcher& cserver::get_event_dispathcher(void)
{
	return csensor_event_dispatcher::get_instance();
}

void *cserver::cb_ipc_start(void *data)
{
	csock::thread_arg_t *arg = (csock::thread_arg_t*)data;
	client_ctx_t *ctx;

	try {
		ctx = new client_ctx_t;
	} catch (...) {
		ERR("Failed to get memory for ctx (%s)\n",__FUNCTION__);
		arg->client_ctx = NULL;
		return (void*)cipc_worker::TERMINATE;
	}

	DBG("IPC Worker started %d\n", pthread_self());

	ctx->client_state = -1;
	ctx->module = NULL;
	ctx->reg_state = 0x00;
	arg->client_ctx = (void*)ctx;
	return (void*)NULL;
}

void *cserver::cb_ipc_stop(void *data)
{
	client_ctx_t *ctx;
	cdata_stream *stream;
	sensor_type sensor = UNKNOWN_SENSOR;
	int client_id;

	DBG("IPC Worker stopped %d\n", pthread_self());
	csock::thread_arg_t *arg = (csock::thread_arg_t*)data;

	ctx = (client_ctx_t*)arg->client_ctx;

	if (!ctx) {
		ERR("Error ctx ptr check (%s)\n",__FUNCTION__);
		arg->client_ctx = NULL;
		return (void*)cipc_worker::TERMINATE;
	}

	client_id = ctx->client_id;

	if (!(ctx->module)) {
		ERR("Error ctx->module ptr check (%s)\n",__FUNCTION__);
		arg->client_ctx = NULL;
		delete ctx;
		return (void*)cipc_worker::TERMINATE;
	}

	stream = (cdata_stream*)ctx->module;

	DBG("Check registeration state : %x", ctx->reg_state);
	if ((ctx->reg_state & 0xFFFF) != 0x00 ) {
		ERR("Connection broken before unregistering events, reg_state : 0x%x", ctx->reg_state);
		const int SENSOR_TYPE_SHIFT = 16;
		sensor = (sensor_type)((ctx->reg_state) >> SENSOR_TYPE_SHIFT);

		event_type_vector event_vec;
		get_client_info_manager().get_registered_events(client_id, sensor, event_vec);

		event_type_vector::iterator it_event;

		it_event = event_vec.begin();

		while (it_event != event_vec.end()) {
			INFO("Unregistering event[0x%x]", *it_event);
			if (!stream->get_processor()->delete_client(*it_event))
				ERR("Unregistering event[0x%x] failed", *it_event);

			++it_event;
		}
	}

	if (ctx->client_state > 0) {				
		ERR("Does not receive cmd_stop before connection broken for %s!! ", stream->name());
		stream->get_processor()->delete_interval(client_id, false);
		if (!stream->get_processor()->stop())
			ERR("Failed to stop %s", stream->name());
	}

	if (sensor) {
		if (get_client_info_manager().has_sensor_record(client_id, sensor)) {
			INFO("Removing sensor[0x%x] record for client_id[%d]", sensor, client_id);
			get_client_info_manager().remove_sensor_record(client_id, sensor);
		}
	}

	arg->client_ctx = NULL;
	delete ctx;

	return (void*)cipc_worker::TERMINATE;
}

void *cserver::cb_ipc_worker(void *data)
{
	cmd_queue_item_t *cmd_item;
	cpacket *packet;
	csock::thread_arg_t *arg = (csock::thread_arg_t*)data;
	csock ipc(arg->client_handle);
	client_ctx_t *ctx;
	ipc.set_on_close(false);

	ctx = (client_ctx_t*)arg->client_ctx;

	DBG("IPC Worker started %d\n", pthread_self());

	try {
		packet = new cpacket(MAX_DATA_STREAM_SIZE);
	} catch (...) {
		return (void*)cipc_worker::TERMINATE;
	}

	DBG("Wait for a packet\n");

	if (ipc.recv(packet->packet(), packet->header_size()) == false) {
		DBG("recv failed\n");
		delete packet;
		return (void*)cipc_worker::TERMINATE;
	}

	if ( !( (unsigned int)packet->payload_size() < MAX_DATA_STREAM_SIZE) ) {
		ERR("Error received packet size check (%s)\n",__FUNCTION__);
		delete packet;
		return (void*)cipc_worker::TERMINATE;
	}

	if (ipc.recv((char*)packet->packet() + packet->header_size(), packet->payload_size()) == false)
	{
		delete packet;
		return (void*)cipc_worker::TERMINATE;
	}

	try {
		cmd_item = new cmd_queue_item_t;
	} catch (...) {
		delete packet;
		return (void*)cipc_worker::TERMINATE;
	}

	cmd_item->packet = packet;
	cmd_item->arg = arg;

	get_instance().command_handler(cmd_item, NULL);
	
	return (void *)cipc_worker::START;
}

void cserver::command_handler(void *cmd_item, void *data)
{
	cmd_queue_item_t *queue_item = (cmd_queue_item_t*)cmd_item;
	cpacket *packet;	
	int cmd= CMD_NONE;
	void *ret;
	
	DBG("Command Handler Invoked\n");

	packet = queue_item->packet;
	cmd = packet->cmd();

	if ( !(cmd > 0 && cmd < CMD_LAST) ) 
	{
		ERR("cmd < 0 or cmd > CMD_LAST");
	}
	else 
	{
		ret = m_cmd_handler[cmd] ? m_cmd_handler[cmd](cmd_item, NULL) : NULL;			
	}			

	delete queue_item->packet;
	delete queue_item;
}

void cserver::sf_main_loop_stop(void)
{
	if(g_mainloop)
	{
		g_main_loop_quit(g_mainloop);
	}
}

int cserver::get_systemd_socket(const char *name)
{
	int type = SOCK_STREAM;
	const int listening = 1;
	size_t length = 0;
	int fd = -1;
	int n;

	if (sd_listen_fds(1) != 1)
		return -1;

	fd = SD_LISTEN_FDS_START + 0;

	if (sd_is_socket_unix(fd, type, listening, name, length) > 0)
		return fd;

	return -1;
}

void cserver::sf_main_loop(void)
{
	csock *ipc = NULL;
	int sock_fd = -1;

	get_event_dispathcher().run();

	g_mainloop = g_main_loop_new(NULL, FALSE);

	try {
		sock_fd = get_systemd_socket(COMMAND_CHANNEL_PATH);
		if (sock_fd >= 0) {
			INFO("Succeeded to get systemd socket");
			ipc = new csock(sock_fd);
		} else {
			ERR("Failed to get systemd socket");
			ipc = new csock(COMMAND_CHANNEL_PATH, 0, true);
		}

	} catch (int ErrNo) {
		ERR("ipc class create fail , errno : %d , errstr : %s\n",ErrNo, strerror(ErrNo));		
		return ;
	}

	ipc->set_worker(cb_ipc_start, cb_ipc_worker, cb_ipc_stop);
	ipc->wait_for_client();

	sd_notify(0, "READY=1");

	g_main_loop_run(g_mainloop);
	g_main_loop_unref(g_mainloop);	

	return;
}

//! End of a file
