cmake_minimum_required(VERSION 2.6)
project(sensord CXX)

INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(server_pkgs REQUIRED vconf glib-2.0 gio-2.0 dlog libsystemd-daemon)

FOREACH(flag ${server_pkgs_LDFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

FOREACH(flag ${server_pkgs_CFLAGS})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
ENDFOREACH(flag)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lrt -ldl -pthread -fPIE")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")

SET(SERVER_SRCS
	dbus_util.cpp
	server.cpp
	command_worker.cpp
	permission_checker.cpp
	main.cpp
)

add_executable(${PROJECT_NAME} ${SERVER_SRCS})

target_link_libraries(${PROJECT_NAME} ${server_pkgs_LDFLAGS} "sensord-server")

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
