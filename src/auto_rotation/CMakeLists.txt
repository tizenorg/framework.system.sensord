cmake_minimum_required(VERSION 2.6)
project(auto_rotation CXX)

SET(SENSOR_NAME auto_rotation_sensor)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)

FOREACH(flag ${auto_rotation_pkgs_LDFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

FOREACH(flag ${auto_rotation_pkgs_CFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

add_library(${SENSOR_NAME} SHARED
	auto_rotation_sensor.cpp
	auto_rotation_alg.cpp
	auto_rotation_alg_emul.cpp
)

target_link_libraries(${SENSOR_NAME} ${auto_rotation_pkgs_LDFLAGS} "-lm")

install(TARGETS ${SENSOR_NAME} DESTINATION lib/sensord)
