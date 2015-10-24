cmake_minimum_required(VERSION 2.6)
project(sensor CXX)

SET(VERSION_MAJOR ${MAJORVER})
SET(VERSION ${FULLVER})

INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(lib_pkgs REQUIRED vconf glib-2.0)

FOREACH(flag ${lib_pkgs_LDFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

FOREACH(flag ${lib_pkgs_CFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden -g -fPIC")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(${PROJECT_NAME} SHARED
	client.cpp
	csensor_event_listener.cpp
	sensor_info_manager.cpp
	csensor_handle_info.cpp
	client_common.cpp
	command_channel.cpp
	poller.cpp
)

target_link_libraries(${PROJECT_NAME} ${lib_pkgs_LDFLAGS} "sensord-share")
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VERSION ${VERSION})

configure_file(${PROJECT_NAME}.pc.in ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc @ONLY)
install(TARGETS ${PROJECT_NAME} DESTINATION lib COMPONENT RuntimeLibraries)
install(FILES sensor_internal_deprecated.h DESTINATION include/sensor/)
install(FILES sensor_internal.h DESTINATION include/sensor/)
install(FILES poller.h DESTINATION include/sensor/)
install(FILES creg_event_info.h DESTINATION include/sensor/)
install(FILES csensor_event_listener.h DESTINATION include/sensor/)
install(FILES sensor_info_manager.h DESTINATION include/sensor/)
install(FILES csensor_handle_info.h DESTINATION include/sensor/)
install(FILES client_common.h DESTINATION include/sensor/)
install(FILES sensor_accel.h DESTINATION include/sensor/)
install(FILES sensor_geomag.h DESTINATION include/sensor/)
install(FILES sensor_geomag_uncal.h DESTINATION include/sensor/)
install(FILES sensor_light.h DESTINATION include/sensor/)
install(FILES sensor_proxi.h DESTINATION include/sensor/)
install(FILES sensor_motion.h DESTINATION include/sensor/)
install(FILES sensor_gyro.h DESTINATION include/sensor/)
install(FILES sensor_gyro_uncal.h DESTINATION include/sensor/)
install(FILES sensor_pressure.h DESTINATION include/sensor/)
install(FILES sensor_pedo.h DESTINATION include/sensor/)
install(FILES sensor_context.h DESTINATION include/sensor/)
install(FILES sensor_flat.h DESTINATION include/sensor/)
install(FILES sensor_bio.h DESTINATION include/sensor/)
install(FILES sensor_bio_hrm.h DESTINATION include/sensor/)
install(FILES sensor_auto_rotation.h DESTINATION include/sensor/)
install(FILES sensor_gravity.h DESTINATION include/sensor/)
install(FILES sensor_linear_accel.h DESTINATION include/sensor/)
install(FILES sensor_orientation.h DESTINATION include/sensor/)
install(FILES sensor_rv.h DESTINATION include/sensor/)
install(FILES sensor_geomagnetic_rv.h DESTINATION include/sensor/)
install(FILES sensor_gaming_rv.h DESTINATION include/sensor/)
install(FILES sensor_rv_raw.h DESTINATION include/sensor/)
install(FILES sensor_pir.h DESTINATION include/sensor/)
install(FILES sensor_pir_long.h DESTINATION include/sensor/)
install(FILES sensor_temperature.h DESTINATION include/sensor/)
install(FILES sensor_humidity.h DESTINATION include/sensor/)
install(FILES sensor_ultraviolet.h DESTINATION include/sensor/)
install(FILES sensor_dust.h DESTINATION include/sensor/)
install(FILES sensor_bio_led_green.h DESTINATION include/sensor/)
install(FILES sensor_tilt.h DESTINATION include/sensor/)
install(FILES ${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
