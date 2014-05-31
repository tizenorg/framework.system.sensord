cmake_minimum_required(VERSION 2.6)
project(sensord_main CXX)

include(FindPkgConfig)
pkg_check_modules(rpkgs REQUIRED glib-2.0 gio-2.0 vconf deviced dlog libxml-2.0 libsystemd-daemon)
add_definitions(${rpkgs_CFLAGS})

# to install pkgconfig setup file.
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION 1.0)

set(PROJECT_MAJOR_VERSION "0")
set(PROJECT_MINOR_VERSION "2")
set(PROJECT_RELEASE_VERSION "1")
set(CMAKE_VERBOSE_MAKEFILE OFF)

add_definitions(-Wall -O3 -omit-frame-pointer)
add_definitions(-DUSE_DLOG_LOG)
#add_definitions(-Wall -g -D_DEBUG)

FIND_PROGRAM(UNAME NAMES uname)
EXEC_PROGRAM("${UNAME}" ARGS "-m" OUTPUT_VARIABLE "ARCH")
IF("${ARCH}" MATCHES "^arm.*")
        ADD_DEFINITIONS("-DTARGET")
        MESSAGE("add -DTARGET")
ELSE("${ARCH}" MATCHES "^arm.*")
        ADD_DEFINITIONS("-DSIMULATOR")
        MESSAGE("add -DSIMULATOR")
ENDIF("${ARCH}" MATCHES "^arm.*")

IF("${ARCH}" MATCHES "^arm.*")
	CONFIGURE_FILE(sf_sensor.conf.in sf_sensor.conf @ONLY)
	CONFIGURE_FILE(sf_filter.conf.in sf_filter.conf @ONLY)
	CONFIGURE_FILE(sf_processor.conf.in sf_processor.conf @ONLY)
	CONFIGURE_FILE(sf_data_stream.conf.in sf_data_stream.conf @ONLY)
	CONFIGURE_FILE(sensors.xml.in sensors.xml @ONLY)
	install(FILES
		sf_sensor.conf
		sf_filter.conf
		sf_processor.conf
		sf_data_stream.conf
		sensors.xml
		DESTINATION etc)
ELSE("${ARCH}" MATCHES "^arm.*")
	CONFIGURE_FILE(sf_sensor_sim.conf.in sf_sensor.conf @ONLY)
	CONFIGURE_FILE(sf_filter_sim.conf.in sf_filter.conf @ONLY)
	CONFIGURE_FILE(sf_processor_sim.conf.in sf_processor.conf @ONLY)
	CONFIGURE_FILE(sf_data_stream_sim.conf.in sf_data_stream.conf @ONLY)
	CONFIGURE_FILE(sensors.xml.in sensors.xml @ONLY)
	install(FILES
		sf_sensor.conf
		sf_filter.conf
		sf_processor.conf
		sf_data_stream.conf
		sensors.xml
		DESTINATION etc)
ENDIF("${ARCH}" MATCHES "^arm.*")

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE DESTINATION share/license RENAME sensord)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE DESTINATION share/license RENAME libsensord)

INSTALL(FILES sensord.rule DESTINATION /etc/smack/accesses2.d)

add_subdirectory(src)
