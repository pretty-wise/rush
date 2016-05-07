include (${CMAKE_SOURCE_DIR}/base/config.cmake)
include (${CMAKE_SOURCE_DIR}/rush/config.cmake)
include (${CMAKE_SOURCE_DIR}/link/common/config.cmake)

#
# punch client
#

set (SOURCES
	punch/include/client.h
	punch/client/client.cpp
	punch/client/punch_client.h
	punch/client/punch_client.cpp
	punch/client/connect_op.h
	punch/client/connect_op.cpp
	punch/client/resolve_op.h
	punch/client/resolve_op.cpp
	punch/client/punch_socket.h
	punch/client/punch_socket.cpp
	punch/common/punch_defs.h
	punch/common/punch_defs.cpp
)

add_library (punch_client ${SOURCES})
target_link_libraries (punch_client base)

#
# punch server
#

include_directories (
	${CMAKE_SOURCE_DIR}/rush/transport/include
)

set (SOURCES
	../include/rush/punch/server.h
	punch/server/server.cpp
	punch/server/punch_server.h
  punch/server/punch_server.cpp
)

add_library (punch_server ${SOURCES})
target_link_libraries (punch_server base)

#
# rush transport
#

set (SOURCES
  ../include/rush/rush.h
  ../include/rush/utils.h
  utils.cpp
	rush.cpp
	endpoint.h
	endpoint.cpp
	congestion.h	
	congestion.cpp
	congestion_buckets.h	
	congestion_buckets.cpp
	rush_context.h
	rush_context.cpp
	ack_info.h
	ack_info.cpp
	rtt.h
	rtt.cpp
	transmission_log.cpp
	bandwidth_limiter.h
	bandwidth_limiter.cpp
)

add_library (rush_transport ${SOURCES})
target_link_libraries (rush_transport base punch_client)

include_directories(
	${CMAKE_SOURCE_DIR}/rush/transport/src/punch/client
)
