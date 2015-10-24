cmake_minimum_required(VERSION 2.6)
project(sensord)

# Setup For pkgconfig File
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(LIBDIR "${PREFIX}/lib")
SET(INCLUDEDIR "${PREFIX}/include")
SET(VERSION ${FULLVER})

CONFIGURE_FILE(
    sensor.pc.in
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor.pc
    @ONLY
)

# Installing files
INSTALL(DIRECTORY ../src/libsensord/ DESTINATION include/sensor
		FILES_MATCHING
		PATTERN "sensor_internal*.h" EXCLUDE
		PATTERN "sensor_*.h"
		)
INSTALL(DIRECTORY include/ DESTINATION include/sensor FILES_MATCHING PATTERN "sensor_internal*.h")
INSTALL(FILES ../src/shared/sensor_common.h DESTINATION include/sensor)
INSTALL(FILES sensor.pc DESTINATION lib/pkgconfig)
INSTALL(FILES ../LICENSE DESTINATION share/license RENAME sensord)
INSTALL(FILES ../LICENSE DESTINATION share/license RENAME libsensord)
