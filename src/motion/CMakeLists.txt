cmake_minimum_required(VERSION 2.6)
project(motion)

SET(SENSOR_NAME motion_sensor)

INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(motion_pkgs REQUIRED vconf)

FOREACH(flag ${motion_pkgs_LDFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

FOREACH(flag ${motion_pkgs_CFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

ADD_DEFINITIONS("-DHWREV_CHECK")

add_definitions(-DMM_MOTION3D_PROJECT)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)

add_library(${SENSOR_NAME} SHARED
	motion_sensor.cpp
)

target_link_libraries(${SENSOR_NAME} ${motion_pkgs_LDFLAGS} "-lm")

install(TARGETS ${SENSOR_NAME} DESTINATION lib/sensord)
