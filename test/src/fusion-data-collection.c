/*
 * sensord
 *
 * Copyright (c) 2014-15 Samsung Electronics Co., Ltd.
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
#include <stdbool.h>
#include <sensor_common.h>
#include <unistd.h>
#include <string.h>

#define MAXSIZE 4

static GMainLoop *mainloop;
FILE* file_output[MAXSIZE];

void callback(sensor_t sensor, unsigned int event_type, sensor_data_t *data, void *user_data)
{
	sensor_type_t sensor_type = event_type >> 16;

	switch (sensor_type) {
	case ACCELEROMETER_SENSOR:
		fprintf(file_output[0], "%6.6f %6.6f %6.6f %lld\n", data->values[0], data->values[1], data->values[2], data->timestamp);
		break;
	case GEOMAGNETIC_SENSOR:
		fprintf(file_output[1], "%6.6f %6.6f %6.6f %lld\n", data->values[0], data->values[1], data->values[2], data->timestamp);
		break;
	case GYROSCOPE_SENSOR:
		fprintf(file_output[2], "%6.6f %6.6f %6.6f %lld\n", data->values[0], data->values[1], data->values[2], data->timestamp);
		break;
	case PROXIMITY_SENSOR:
		fprintf(file_output[MAXSIZE-1], "%6.6f %lld\n", data->values[0], data->timestamp);
		break;
	default:
		return;
	}
}

void usage()
{
	printf("Usage : ./one-client-multiple-sensors <interval>\n\n");

	printf("interval:\n");
	printf("The sampling interval in ms.\n");
	exit(-1);
}

int main(int argc, char **argv)
{
	int interval;

	if (argc == 2) {
		interval = atoi(argv[1]);
		if (interval <= 0)
			usage();
	}
	else
		usage();

	int i;

	int handle[MAXSIZE];
	int result[MAXSIZE], start_handle[MAXSIZE];
	unsigned int event[MAXSIZE];
	int sensors[MAXSIZE];

	sensors[0] = ACCELEROMETER_SENSOR;
	sensors[1] = GEOMAGNETIC_SENSOR;
	sensors[2] = GYROSCOPE_SENSOR;
	sensors[MAXSIZE-1] = PROXIMITY_SENSOR;

	mainloop = g_main_loop_new(NULL, FALSE);

	char file_name[50];

	for (i = 0; i < MAXSIZE; i++) {
		sprintf(file_name, "output_%d", sensors[i]);
		file_output[i] = fopen(file_name, "w+");
		sensor_t sensor = sensord_get_sensor(sensors[i]);
		handle[i] = sensord_connect(sensor);
		event[i] = (sensors[i] << 16) | 0x0001;
		result[i] = sensord_register_event(handle[i], event[i], interval, 0, callback, NULL);

		if (result[i] < 0) {
			printf("error: unable to register sensor\n");
			return -1;
		}
		start_handle[i] = sensord_start(handle[i], 1);

		if (start_handle[i] < 0) {
			printf("error: unable to start handle\n");
			sensord_unregister_event(handle[i], event[i]);
			sensord_disconnect(handle[i]);
			return -1;
		}
	}

	g_main_loop_run(mainloop);
	g_main_loop_unref(mainloop);

	for (i = 0; i < MAXSIZE; i++) {
		sensord_unregister_event(handle[i], event[i]);
		sensord_stop(handle[i]);
		sensord_disconnect(handle[i]);
	}

	return 0;
}
