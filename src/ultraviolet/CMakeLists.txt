cmake_minimum_required(VERSION 2.6)
project(ultraviolet CXX)

SET(SENSOR_NAME ultraviolet_sensor)
SET(SENSOR_HAL_NAME ultraviolet_sensor_hal)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)

FOREACH(flag ${uv_pkgs_LDFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

FOREACH(flag ${uv_pkgs_CFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

add_library(${SENSOR_NAME} SHARED
		ultraviolet_sensor.cpp
		)

add_library(${SENSOR_HAL_NAME} SHARED
		ultraviolet_sensor_hal.cpp
		)

target_link_libraries(${SENSOR_NAME} ${uv_pkgs_LDFLAGS} "-lm")
target_link_libraries(${SENSOR_HAL_NAME} ${uv_pkgs_LDFLAGS})

install(TARGETS ${SENSOR_NAME} DESTINATION lib/sensord)
install(TARGETS ${SENSOR_HAL_NAME} DESTINATION lib/sensord)
