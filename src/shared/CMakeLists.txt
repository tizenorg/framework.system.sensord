cmake_minimum_required(VERSION 2.6)
project(sf_common CXX)
# to install pkgconfig setup file.
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION 1.0)

#set(CMAKE_INSTALL_PREFIX "$ENV{DATAFS}")
set(PROJECT_MAJOR_VERSION "0")
set(PROJECT_MINOR_VERSION "2")
set(PROJECT_RELEASE_VERSION "1")
set(CMAKE_VERBOSE_MAKEFILE OFF)

include(FindPkgConfig)
pkg_check_modules(rpkgs REQUIRED dlog libxml-2.0)
add_definitions(${rpkgs_CFLAGS})

add_definitions(-DPREFIX="${CMAKE_INSTALL_PREFIX}")
add_definitions(-DLOCALEDIR="$ENV{DATAFS}/share/locale")
add_definitions(-DFACTORYFS="$ENV{FACTORYFS}")
add_definitions(-DDATAFS="$ENV{DATAFS}")
add_definitions(-Wall -std=gnu++0x)
add_definitions(-DUSE_DLOG_LOG)
#add_definitions(-DX1_PROF)
#add_definitions(-D_GETTEXT)
#add_definitions(-Wall -O3 -omit-frame-pointer)
#add_definitions(-D_DEFAULT_FONT_NAME="Vera")
#add_definitions(-Wall -g -DUSE_FILE_DEBUG)
#add_definitions(-Wall -g -D_DEBUG)
#add_definitions(-Wl,--as-needed -Wl,-O1)
#add_definitions(-finstrument-functions)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(${PROJECT_NAME} SHARED
	csensor_module.cpp
	cfilter_module.cpp
	cprocessor_module.cpp
	clist.cpp
	cmodule.cpp
	cbase_lock.cpp
	cmutex.cpp
	crw_lock.cpp
	cobject_type.cpp
	cpacket.cpp
	csock.cpp
	cworker.cpp
	cipc_worker.cpp
	common.cpp
	cconfig.cpp
	csensor_event_queue.cpp
	cinterval_info_list.cpp
	cevent_socket.cpp
)

target_link_libraries(${PROJECT_NAME} ${rpkgs_LDFLAGS} "-lrt -ldl -pthread")
configure_file(${PROJECT_NAME}.pc.in ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc @ONLY)

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(FILES ${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
install(FILES
	sf_common.h
	csensor_module.h
	cfilter_module.h
	cprocessor_module.h
	clist.h
	cmodule.h
	cbase_lock.h
	cmutex.h
	crw_lock.h
	cobject_type.h
	cpacket.h
	csock.h
	cworker.h
	cipc_worker.h
	common.h
	cutil.h
	cconfig.h
	csensor_event_queue.h
	cinterval_info_list.h
	cevent_socket.h
	DESTINATION include/${PROJECT_NAME}
)
