/*
 * sensord
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
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
#include <time.h>
#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <sensor_internal.h>
#include <sensor_common.h>
#include <stdbool.h>
#include <sensor_common.h>
#include <unistd.h>

#define DEFAULT_EVENT_INTERVAL 100

static GMainLoop *mainloop;
FILE *fp;

void callback(sensor_t sensor, unsigned int event_type, sensor_data_t *data, void *user_data)
{
	g_main_loop_quit(mainloop);
}

bool check_sensor_api(unsigned int event_type, int cond_value)
{
	int result, handle;

	mainloop = g_main_loop_new(NULL, FALSE);

	sensor_type_t sensor_type = event_type >> 16;
	sensor_t sensor = sensord_get_sensor(sensor_type);

	handle = sensord_connect(sensor);

	if (handle < 0) {
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_connect\n", sensor_type, event_type);
		return false;
	}

	bool is_supported;
	bool result_boolean = sensord_is_supported_event_type(sensor, event_type, &is_supported);
	if (!result_boolean && !is_supported) {
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_is_supported_event\n", sensor_type, event_type);
		return false;
	}

	int output;
	result_boolean = sensord_get_min_interval(sensor, &output);
	if (!result_boolean) {
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_get_min_interval\n", sensor_type, event_type);
		return false;
	}

	float output3;
	result_boolean = sensord_get_resolution(sensor, &output3);
	if (!result_boolean) {
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_get_resolution\n", sensor_type, event_type);
		return false;
	}

	result_boolean = sensord_get_max_range(sensor, &output3);
	if (!result_boolean) {
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_get_max_range\n", sensor_type, event_type);
		return false;
	}

	result_boolean = sensord_get_min_range(sensor, &output3);
	if (!result_boolean) {
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_get_min_range\n", sensor_type, event_type);
		return false;
	}

	sensor_privilege_t output4;
	result_boolean = sensord_get_privilege(sensor, &output4);
	if (!result_boolean) {
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_get_privilege\n", sensor_type, event_type);
		return false;
	}

	const char* result_char = sensord_get_vendor(sensor);
	if (!result_char) {
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_get_vendor\n", sensor_type, event_type);
		return false;
	}

	result_char = sensord_get_name(sensor);
	if (!result_char) {
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_get_name\n", sensor_type, event_type);
		return false;
	}

	sensor_type_t output_type;
	result_boolean = sensord_get_type(sensor, &output_type);
	if (!result_boolean) {
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_get_type\n", sensor_type, event_type);
		return false;
	}

	unsigned int *output2;
	result_boolean = sensord_get_supported_event_types(sensor, &output2, &output);
	if (!result_boolean) {
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_get_supported_event_types\n", sensor_type, event_type);
		return false;
	}

	sensor_t *output_list;
	result_boolean = sensord_get_sensor_list(sensor_type, &output_list, &output);
	if (!result_boolean) {
		free(output2);
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_get_sensor_list\n", sensor_type, event_type);
		return false;
	}

	result = sensord_register_event(handle, event_type, cond_value, 0, callback, NULL);

	if (result < 0) {
		free(output2);
		free(output_list);
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_register_event\n", sensor_type, event_type);
		return false;
	}

	result_boolean = sensord_start(handle, 1);

	if (!result_boolean) {
		sensord_unregister_event(handle, event_type);
		sensord_disconnect(handle);
		free(output2);
		free(output_list);
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_start\n", sensor_type, event_type);
		return false;
	}

	sensor_data_t data;
	result_boolean = sensord_get_data(handle, event_type, &data);
	if (!result_boolean) {
		sensord_unregister_event(handle, event_type);
		sensord_disconnect(handle);
		free(output2);
		free(output_list);
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_get_data\n", sensor_type, event_type);
		return false;
	}

	g_main_loop_run(mainloop);
	g_main_loop_unref(mainloop);

	result_boolean = sensord_change_event_interval(handle, event_type, 101);
	if (!result_boolean) {
		free(output2);
		free(output_list);
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_change_event_interval\n", sensor_type, event_type);
		return false;
	}

	result_boolean = sensord_set_option(handle, SENSOR_OPTION_ON_IN_SCREEN_OFF);
	if (!result_boolean){
		free(output2);
		free(output_list);
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_change_sensor_option\n", sensor_type, event_type);
		return false;
	}

	result_boolean = sensord_unregister_event(handle, event_type);

	if (!result_boolean) {
		free(output2);
		free(output_list);
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_unregister_event\n", sensor_type, event_type);
		return false;
	}

	result_boolean = sensord_stop(handle);

	if (!result_boolean) {
		free(output2);
		free(output_list);
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_stop\n", sensor_type, event_type);
		return false;
	}

	result_boolean = sensord_disconnect(handle);

	if (!result_boolean) {
		free(output2);
		free(output_list);
		fprintf(fp, "Sensor - %d, event - %d, failed at sensord_disconnect\n", sensor_type, event_type);
		return false;
	}

	free(output2);
	free(output_list);

	return true;
}

int main(int argc, char **argv)
{
	bool result;

	int interval = DEFAULT_EVENT_INTERVAL;
	if (argc == 2)
		interval = atof(argv[1]);

	fp = fopen("api-test.output", "w+");

	result = check_sensor_api(ACCELEROMETER_EVENT_RAW_DATA_REPORT_ON_TIME, interval);
	fprintf(fp, "Accelerometer - RAW_DATA_REPORT_ON_TIME - %d\n", result);

	result = check_sensor_api(GEOMAGNETIC_EVENT_RAW_DATA_REPORT_ON_TIME, interval);
	fprintf(fp, "Geomagnetic - RAW_DATA_REPORT_ON_TIME - %d\n", result);

	result = check_sensor_api(GRAVITY_EVENT_RAW_DATA_REPORT_ON_TIME, interval);
	fprintf(fp, "Gravity - RAW_DATA_REPORT_ON_TIME - %d\n", result);

	result = check_sensor_api(GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME, interval);
	fprintf(fp, "Gyroscope - RAW_DATA_REPORT_ON_TIME - %d\n", result);

	result = check_sensor_api(LIGHT_EVENT_LUX_DATA_REPORT_ON_TIME, interval);
	fprintf(fp, "Light - RAW_DATA_REPORT_ON_TIME - %d\n", result);

	result = check_sensor_api(LINEAR_ACCEL_EVENT_RAW_DATA_REPORT_ON_TIME, interval);
	fprintf(fp, "Linear Accel - RAW_DATA_REPORT_ON_TIME - %d\n", result);

	result = check_sensor_api(ORIENTATION_EVENT_RAW_DATA_REPORT_ON_TIME, interval);
	fprintf(fp, "Orientation - RAW_DATA_REPORT_ON_TIME - %d\n", result);

	result = check_sensor_api(PRESSURE_EVENT_RAW_DATA_REPORT_ON_TIME, interval);
	fprintf(fp, "Pressure - RAW_DATA_REPORT_ON_TIME - %d\n", result);

	result = check_sensor_api(ROTATION_VECTOR_EVENT_RAW_DATA_REPORT_ON_TIME, interval);
	fprintf(fp, "Rotation Vector - RAW_DATA_REPORT_ON_TIME - %d\n", result);

	result = check_sensor_api(GEOMAGNETIC_RV_EVENT_RAW_DATA_REPORT_ON_TIME, interval);
	fprintf(fp, "Geomagnetic RV - RAW_DATA_REPORT_ON_TIME - %d\n", result);

	result = check_sensor_api(GAMING_RV_EVENT_RAW_DATA_REPORT_ON_TIME, interval);
	fprintf(fp, "Gaming RV - RAW_DATA_REPORT_ON_TIME - %d\n", result);

	result = check_sensor_api(GYROSCOPE_UNCAL_EVENT_RAW_DATA_REPORT_ON_TIME, interval);
	fprintf(fp, "Gyro Uncal - RAW_DATA_REPORT_ON_TIME - %d\n", result);

	result = check_sensor_api(TILT_EVENT_RAW_DATA_REPORT_ON_TIME, interval);
	fprintf(fp, "Tilt - RAW_DATA_REPORT_ON_TIME - %d\n", result);

	printf("Logs printed in ./api-test.output\n");
	fclose(fp);
	return 0;
}
