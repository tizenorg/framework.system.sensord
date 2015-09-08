cmake_minimum_required(VERSION 2.6)
project(bio_hrm CXX)

SET(SENSOR_PHY_NAME bio_hrm_phy_sensor)
SET(SENSOR_VIRT_NAME bio_hrm_virt_sensor)
SET(SENSOR_HAL_NAME bio_hrm_sensor_hal)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)

INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(bio_hrm_pkgs REQUIRED vconf)

FOREACH(flag ${bio_hrm_pkgs_LDFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

FOREACH(flag ${bio_hrm_pkgs_CFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

add_library(${SENSOR_PHY_NAME} SHARED
	${SENSOR_PHY_NAME}.cpp
)

add_library(${SENSOR_VIRT_NAME} SHARED
	hrm_alg.cpp
	hrm_alg_maxim.cpp
	hrm_alg_adi.cpp
	${SENSOR_VIRT_NAME}.cpp
)

add_library(${SENSOR_HAL_NAME} SHARED
	${SENSOR_HAL_NAME}.cpp
)

target_link_libraries(${SENSOR_PHY_NAME} ${bio_hrm_pkgs_LDFLAGS} "-lm")
target_link_libraries(${SENSOR_VIRT_NAME} ${bio_hrm_pkgs_LDFLAGS} "-lm")
target_link_libraries(${SENSOR_HAL_NAME} ${bio_hrm_pkgs_LDFLAGS})

install(TARGETS ${SENSOR_PHY_NAME} DESTINATION lib/sensord)
install(TARGETS ${SENSOR_VIRT_NAME} DESTINATION lib/sensord)
install(TARGETS ${SENSOR_HAL_NAME} DESTINATION lib/sensord)
