CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
project(sensor-tc C)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(VERSION 1.0)

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED glib-2.0)

add_definitions(${rpkgs_CFLAGS})
add_definitions(-DPREFIX="${CMAKE_INSTALL_PREFIX}")

configure_file(${PROJECT_NAME}.pc.in ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc @ONLY)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)
include_directories(${CMAKE_SOURCE_DIR}/src/shared)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

link_directories(../src/libsensord/)

add_executable(api-test src/api-test.c)
add_executable(sensor-test src/sensor-test.c src/check-sensor.c)
add_executable(performance-test src/performance-test.c src/check-sensor.c)
add_executable(fusion-data-collection src/fusion-data-collection.c)

SET_TARGET_PROPERTIES(api-test PROPERTIES LINKER_LANGUAGE C)
SET_TARGET_PROPERTIES(sensor-test PROPERTIES LINKER_LANGUAGE C)
SET_TARGET_PROPERTIES(performance-test PROPERTIES LINKER_LANGUAGE C)
SET_TARGET_PROPERTIES(fusion-data-collection PROPERTIES LINKER_LANGUAGE C)

target_link_libraries(api-test glib-2.0 dlog sensor)
target_link_libraries(sensor-test glib-2.0 dlog sensor)
target_link_libraries(performance-test glib-2.0 dlog sensor)
target_link_libraries(fusion-data-collection glib-2.0 dlog sensor)

INSTALL(TARGETS api-test DESTINATION /usr/bin/)
INSTALL(TARGETS sensor-test DESTINATION /usr/bin/)
INSTALL(TARGETS performance-test DESTINATION /usr/bin/)
INSTALL(TARGETS fusion-data-collection DESTINATION /usr/bin/)
