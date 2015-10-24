cmake_minimum_required(VERSION 2.6)
project(geomagnetic_rv CXX)

SET(SENSOR_NAME geomagnetic_rv_sensor)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)
include_directories(${CMAKE_SOURCE_DIR}/src/sensor_fusion)

FOREACH(flag ${geomagnetic_rv_pkgs_LDFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

FOREACH(flag ${geomagnetic_rv_pkgs_CFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

add_library(${SENSOR_NAME} SHARED
		geomagnetic_rv_sensor.cpp
)

target_link_libraries(${SENSOR_NAME} ${geomagnetic_rv_pkgs_LDFLAGS} "-lm")

install(TARGETS ${SENSOR_NAME} DESTINATION lib/sensord)
