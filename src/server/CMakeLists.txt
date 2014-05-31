cmake_minimum_required(VERSION 2.6)
project(sf_server CXX)

#add_definitions(-Wall -g -DUSE_FILE_DEBUG)
#add_definitions(-Wall -g -D_DEBUG)
#add_definitions(-Wall -g -pg)
add_definitions(-Wall -std=gnu++0x)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libsensord)

SET(SERVER_SRCS
	cserver.cpp
	csensor_catalog.cpp
	cfilter_catalog.cpp
	cprocessor_catalog.cpp
	cdata_stream.cpp
	resource_str.cpp
	ctrim.cpp
	ccatalog.cpp
	main.cpp
	csensor_event_dispatcher.cpp
	csensor_usage.cpp
	cclient_sensor_record.cpp
	cclient_info_manager.cpp
)

add_executable(${PROJECT_NAME} ${SERVER_SRCS})

target_link_libraries(${PROJECT_NAME} ${rpkgs_LDFLAGS} "sf_common")
set(CMAKE_CXX_FLAGS "-lrt -ldl -pthread ${CMAKE_CXX_FLAGS}")


install(TARGETS ${PROJECT_NAME} DESTINATION bin)
