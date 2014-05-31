cmake_minimum_required(VERSION 2.6)
project(sensor CXX)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION_MAJOR 1)
SET(VERSION "${VERSION_MAJOR}.1.0")

include(FindPkgConfig)
pkg_check_modules(rpkgs REQUIRED vconf glib-2.0)
add_definitions(${rpkgs_CFLAGS})

#add_definitions(-Wall -O3 -omit-frame-pointer -lm)
#add_definitions(-Wall -g -lma -DUSE_FILE_DEBUG)
#add_definitions(-D_DEBUG)
add_definitions(-Wall -g -lma -DUSE_DLOG_LOG -std=c++0x)
#add_definitions(-fvisibility=hidden -lm -DUSE_DLOG_LOG)
#add_definitions(-fvisibility=hidden -lm)

FIND_PROGRAM(UNAME NAMES uname)
EXEC_PROGRAM("${UNAME}" ARGS "-m" OUTPUT_VARIABLE "ARCH")
IF("${ARCH}" STREQUAL "arm")
#        ADD_DEFINITIONS("-DTARGET -DHWREV_CHECK -DUSE_MPU3050_GYRO")
	ADD_DEFINITIONS("-DTARGET -DHWREV_CHECK")
        MESSAGE("add -DTARGET")
ELSE("${ARCH}" STREQUAL "arm")
        ADD_DEFINITIONS("-DSIMULATOR")
        MESSAGE("add -DSIMULATOR")
ENDIF("${ARCH}" STREQUAL "arm")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(${PROJECT_NAME} SHARED
	client.cpp
	csensor_event_listener.cpp
	csensor_handle_info.cpp
	client_common.cpp
	command_channel.cpp
	poller.cpp
)

#add_dependencies(${PROJECT_NAME} sf_common)
# to install pkgconfig setup file.

target_link_libraries(${PROJECT_NAME} ${rpkgs_LDFLAGS} ${GLES_LDFLAGS} "sf_common")
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES SOVERSION ${VERSION_MAJOR})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VERSION ${VERSION})

configure_file(${PROJECT_NAME}.pc.in ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc @ONLY)

#install(DIRECTORY include/ DESTINATION include/ FILES_MATCHING PATTERN "*.h")
install(TARGETS ${PROJECT_NAME} DESTINATION lib COMPONENT RuntimeLibraries)

install(FILES sensor.h DESTINATION include/sensor/)
install(FILES poller.h DESTINATION include/sensor/)
install(FILES creg_event_info.h DESTINATION include/sensor/)
install(FILES csensor_event_listener.h DESTINATION include/sensor/)
install(FILES csensor_handle_info.h DESTINATION include/sensor/)
install(FILES client_common.h DESTINATION include/sensor/)
install(FILES sensor_accel.h DESTINATION include/sensor/)
install(FILES sensor_geomag.h DESTINATION include/sensor/)
install(FILES sensor_light.h DESTINATION include/sensor/)
install(FILES sensor_proxi.h DESTINATION include/sensor/)
install(FILES sensor_motion.h DESTINATION include/sensor/)
install(FILES sensor_gyro.h DESTINATION include/sensor/)
install(FILES sensor_barometer.h DESTINATION include/sensor/)
install(FILES sensor_fusion.h DESTINATION include/sensor/)
install(FILES sensor_pedo.h DESTINATION include/sensor/)
install(FILES sensor_context.h DESTINATION include/sensor/)
install(FILES sensor_flat.h DESTINATION include/sensor/)
install(FILES sensor_bio.h DESTINATION include/sensor/)
install(FILES sensor_bio_hrm.h DESTINATION include/sensor/)
install(FILES ${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
