cmake_minimum_required(VERSION 2.6)
project(bio_led_green CXX)

SET(SENSOR_NAME bio_led_green_sensor)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)

INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(bio_led_green_pkgs REQUIRED vconf)

FOREACH(flag ${bio_led_green_pkgs_LDFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

FOREACH(flag ${bio_led_green_pkgs_CFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

add_library(${SENSOR_NAME} SHARED
		bio_led_green_sensor.cpp
		)

target_link_libraries(${SENSOR_NAME} ${bio_led_green_pkgs_LDFLAGS} "-lm")

install(TARGETS ${SENSOR_NAME} DESTINATION lib/sensord)
