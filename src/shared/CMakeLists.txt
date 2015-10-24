cmake_minimum_required(VERSION 2.6)
project(sf_common CXX)

INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(shared_pkgs REQUIRED vconf dlog libxml-2.0)

FOREACH(flag ${shared_pkgs_LDFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

FOREACH(flag ${shared_pkgs_CFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)

add_library(sensord-server SHARED
	crw_lock.cpp
	worker_thread.cpp
	cconfig.cpp
	csensor_config.cpp
	cvirtual_sensor_config.cpp
	csensor_event_queue.cpp
	csensor_event_dispatcher.cpp
	csensor_usage.cpp
	cclient_info_manager.cpp
	cclient_sensor_record.cpp
	cinterval_info_list.cpp
	batch_info_list.cpp
	sensor_plugin_loader.cpp
	sensor_hal.cpp
	sensor_base.cpp
	physical_sensor.cpp
	virtual_sensor.cpp
	sensor_fusion.cpp
)

add_library(sensord-share SHARED
	cpacket.cpp
	csocket.cpp
	cbase_lock.cpp
	cmutex.cpp
	common.cpp
	sensor_info.cpp
)

target_link_libraries(sensord-server ${shared_pkgs_LDFLAGS} "-lrt -ldl -pthread" "sensord-share")
target_link_libraries(sensord-share ${shared_pkgs_LDFLAGS} "-lrt -ldl -pthread")
configure_file(sensord-server.pc.in ${CMAKE_CURRENT_SOURCE_DIR}/sensord-server.pc @ONLY)
configure_file(${PROJECT_NAME}.pc.in ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc @ONLY)

install(TARGETS sensord-server DESTINATION lib)
install(TARGETS sensord-share DESTINATION lib)
install(FILES sensord-server.pc DESTINATION lib/pkgconfig)
install(FILES ${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
install(FILES
	sensor_fusion.h
	crw_lock.h
	worker_thread.h
	cconfig.h
	csensor_config.h
	cvirtual_sensor_config.h
	csensor_event_queue.h
	cinterval_info_list.h
	batch_info_list.h
	sensor_plugin_loader.h
	sensor_hal.h
	sensor_base.h
	physical_sensor.h
	virtual_sensor.h
	sensor_fusion.h
	sf_common.h
	cpacket.h
	csocket.h
	cbase_lock.h
	cmutex.h
	common.h
	sensor_info.h
	DESTINATION include/${PROJECT_NAME}
)

install(FILES
	sensor_common.h
	DESTINATION include/sensor
)

