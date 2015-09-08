cmake_minimum_required(VERSION 2.6)
project(pir CXX)

SET(SENSOR_NAME pir_sensor)
SET(SENSOR_HAL_NAME pir_sensor_hal)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)

FOREACH(flag ${pir_pkgs_LDFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

FOREACH(flag ${pir_pkgs_CFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

add_library(${SENSOR_NAME} SHARED
	pir_sensor.cpp
)

add_library(${SENSOR_HAL_NAME} SHARED
        pir_sensor_hal.cpp
)

target_link_libraries(${SENSOR_NAME} ${pir_pkgs_LDFLAGS} "-lm")
target_link_libraries(${SENSOR_HAL_NAME} ${pir_pkgs_LDFLAGS})

install(TARGETS ${SENSOR_NAME} DESTINATION lib/sensord)
install(TARGETS ${SENSOR_HAL_NAME} DESTINATION lib/sensord)
