cmake_minimum_required(VERSION 2.6)
project(bio CXX)

SET(SENSOR_NAME bio_sensor)
SET(SENSOR_HAL_NAME bio_sensor_hal)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)

FOREACH(flag ${bio_pkgs_LDFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

FOREACH(flag ${bio_kgs_CFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

add_definitions(-Iinclude)

add_library(${SENSOR_NAME} SHARED
	bio_sensor.cpp
)

add_library(${SENSOR_HAL_NAME} SHARED
	bio_data_reader.cpp
	bio_data_reader_standard.cpp
	bio_data_reader_adi.cpp
	bio_sensor_hal.cpp
)

target_link_libraries(${SENSOR_NAME} ${bio_pkgs_LDFLAGS} "-lm")
target_link_libraries(${SENSOR_HAL_NAME} ${bio_pkgs_LDFLAGS})

install(TARGETS ${SENSOR_NAME} DESTINATION lib/sensord)
install(TARGETS ${SENSOR_HAL_NAME} DESTINATION lib/sensord)
