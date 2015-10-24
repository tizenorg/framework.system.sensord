cmake_minimum_required(VERSION 2.6)
project(sensord_main CXX)

# Setup For pkgconfig File
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "${PREFIX}/bin")
SET(LIBDIR "${PREFIX}/lib")
SET(INCLUDEDIR "${PREFIX}/include")
SET(VERSION 1.0)

# Common Options
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2 -omit-frame-pointer -std=gnu++0x")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdata-sections -ffunction-sections")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-section -Wl,--print-gc-section")
MESSAGE("FLAGS: ${CMAKE_CXX_FLAGS}")
MESSAGE("FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")

add_definitions(-DUSE_DLOG_LOG)

# Internal Debugging Options
#add_definitions(-Wall -g -D_DEBUG)

# Installing files
CONFIGURE_FILE(sensor_plugins.xml.in sensor_plugins.xml @ONLY)
CONFIGURE_FILE(sensors.xml.in sensors.xml @ONLY)
CONFIGURE_FILE(virtual_sensors.xml.in virtual_sensors.xml @ONLY)
install(FILES
	sensor_plugins.xml
	sensors.xml
	virtual_sensors.xml
	DESTINATION etc)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE DESTINATION share/license RENAME sensord)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE DESTINATION share/license RENAME libsensord)

INSTALL(FILES sensord.efl DESTINATION /etc/smack/accesses.d)
IF("${TIZEN_ZONE_ENABLED}" STREQUAL "ON")
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/packaging/sensord.res DESTINATION /etc/vasum/vsmzone.resource)
ENDIF()

# Sub-directory
add_subdirectory(src)

IF("${TEST_SUITE}" STREQUAL "ON")
add_subdirectory(test)
ENDIF()
