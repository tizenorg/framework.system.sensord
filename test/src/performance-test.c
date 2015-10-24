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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sensor_common.h>

#include "check-sensor.h"

void usage()
{
	printf("Usage : ./performance-test <TIMEOUT> <interval>(optional)\n\n");

	printf("TIMEOUT:\n");
	printf("time for which the parallel sensor test cases should run\n");

	printf("interval:\n");
	printf("The time interval should be entered based on the sampling frequency supported by accelerometer driver on the device in ms.\n");
	printf("If no value for sensor is entered default value by the driver will be used.\n");
}

int main(int argc, char** argv)
{
	pid_t b = 1;

	int i = 0;
	int interval = DEFAULT_EVENT_INTERVAL;
	int TIMEOUT = 10; //in seconds for which all the sensor tests should run

	if(argc < 2) {
		usage();
		return -1;
	}
	else if(argc == 2){
		TIMEOUT = atoi(argv[1]);
		if (TIMEOUT == 0) {
			usage();
			return -1;
		}
	}
	else {
		TIMEOUT = atoi(argv[1]);
		interval = atoi(argv[2]);
		if (TIMEOUT == 0 || interval == 0) {
			usage();
			return -1;
		}
	}

	//make an array of size MAX and fill it with all the sensors needed to run
	int MAX = 6;
	pid_t pids[MAX];
	sensor_type_t sensor[MAX];

	//Update the value of MAX and add more sensors here to test more sensors in parallel
	sensor[0] = ACCELEROMETER_SENSOR;
	sensor[1] = GYROSCOPE_SENSOR;
	sensor[2] = GEOMAGNETIC_SENSOR;
	sensor[3] = PRESSURE_SENSOR;
	sensor[4] = PROXIMITY_SENSOR;
	sensor[MAX-1] = LIGHT_SENSOR;

	while (i < MAX) {
		if (b > 0) {
			b = fork();
			if (b == -1) perror("Fork failed\n");
			else if (b == 0) {
				break;
			}
			pids[i] = b;
			i++;
		}
	}

	if (i < MAX) {
		// call the sensord test tc-common for a sensor.
		int event = (sensor[i] << 16) | 0x0001;
		check_sensor(sensor[i], event, interval);
	}
	else {
		// Main Parent Child. Waits for TIMEOUT and then kills all child processes.
		sleep (TIMEOUT);
		int j = 0;

		for (j = 0; j < MAX; j++) {
			char command[100];
			sprintf(command, "kill %d", pids[j]);
			if (system(command) == -1)
				return -1;
		}
	}

	return 0;
}
