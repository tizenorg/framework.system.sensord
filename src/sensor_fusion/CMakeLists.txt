cmake_minimum_required(VERSION 2.6)
project(sensor_fusion CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)

add_definitions(-Wall -O3 -omit-frame-pointer)
#add_definitions(-Wall -g -D_DEBUG)
add_definitions(-DUSE_DLOG_LOG)
add_definitions(-Iinclude)

SET(SENSOR_FUSION_NAME sensor_fusion)

add_library(${SENSOR_FUSION_NAME} SHARED
		euler_angles.cpp
		matrix.cpp
		orientation_filter.cpp
		quaternion.cpp
		rotation_matrix.cpp
		sensor_data.cpp
		vector.cpp
		)

target_link_libraries(${SENSOR_FUSION_NAME} ${sensor_fusion_LDFLAGS} "-lm")

install(TARGETS ${SENSOR_FUSION_NAME} DESTINATION lib/sensord)
