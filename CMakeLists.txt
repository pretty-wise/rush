cmake_minimum_required (VERSION 2.6)
project (rush)
set (rush_VERSION_MAJOR 0)
set (rush_VERSION_MINOR 1)

include (settings.cmake)

set(TARGET_EXPORT_NAME "rush-targets")
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries.")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for binaries.")
set(INSTALL_INCLUDE_DIR include CACHE PATH "Installation directory for includes.")
set(INSTALL_CMAKE_DIR cmake CACHE PATH "Installation directory for cmake files.")

# Make relative paths absolute (needed later on)
foreach(p LIB BIN INCLUDE CMAKE)
	set(var INSTALL_${p}_DIR)
	if(NOT IS_ABSOLUTE "${${var}}")
		set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
	endif()
endforeach()

find_package(base REQUIRED)

if(NOT BASE_FOUND)
	message(FATAL_ERROR "base library not found. searched in: ${base_CONSIDERED_CONFIGS}")
endif()

include_directories (
	${BASE_INCLUDES}
)

#
# punch client
#
set (SOURCES
	transport/src/punch/include/client.h
	transport/src/punch/client/client.cpp
	transport/src/punch/client/punch_client.h
	transport/src/punch/client/punch_client.cpp
	transport/src/punch/client/connect_op.h
	transport/src/punch/client/connect_op.cpp
	transport/src/punch/client/resolve_op.h
	transport/src/punch/client/resolve_op.cpp
	transport/src/punch/client/punch_socket.h
	transport/src/punch/client/punch_socket.cpp
	transport/src/punch/common/punch_defs.h
	transport/src/punch/common/punch_defs.cpp
)

add_library (punch_client ${SOURCES})
target_link_libraries (punch_client base)

#
# punch server
#
include_directories (
	${CMAKE_SOURCE_DIR}/transport/include
)

set (SOURCES
	transport/include/rush/punch/server.h
	transport/src/punch/server/server.cpp
	transport/src/punch/server/punch_server.h
  transport/src/punch/server/punch_server.cpp
)

add_library (punch_server ${SOURCES})
target_link_libraries (punch_server base)

#
# rush transport
#
set (SOURCES
  transport/include/rush/rush.h
  transport/include/rush/utils.h
  transport/src/utils.cpp
	transport/src/rush.cpp
	transport/src/endpoint.h
	transport/src/endpoint.cpp
	transport/src/congestion.h	
	transport/src/congestion.cpp
	transport/src/congestion_buckets.h	
	transport/src/congestion_buckets.cpp
	transport/src/rush_context.h
	transport/src/rush_context.cpp
	transport/src/ack_info.h
	transport/src/ack_info.cpp
	transport/src/rtt.h
	transport/src/rtt.cpp
	transport/src/transmission_log.cpp
	transport/src/bandwidth_limiter.h
	transport/src/bandwidth_limiter.cpp
)

add_library (rush_transport ${SOURCES})
target_link_libraries (rush_transport base punch_client)

install(TARGETS punch_client punch_server rush_transport 
	DESTINATION ${INSTALL_LIB_DIR}
	EXPORT ${TARGET_EXPORT_NAME}
)

export (TARGETS punch_client punch_server rush_transport
	FILE "${CMAKE_BINARY_DIR}/${TARGET_EXPORT_NAME}.cmake"
)
export (PACKAGE rush)

file(RELATIVE_PATH RELATIVE_INCLUDE_DIR "${INSTALL_CMAKE_DIR}"
	   "${INSTALL_INCLUDE_DIR}"
)
set(CONF_INCLUDE_DIRS "\${RUSH_CMAKE_DIR}/${RELATIVE_INCLUDE_DIR}")

configure_file(${CMAKE_SOURCE_DIR}/cmake/config.cmake.in
	${CMAKE_BINARY_DIR}/rush-config.cmake @ONLY
)

install(DIRECTORY transport/include/
	DESTINATION ${INSTALL_INCLUDE_DIR}
)

install(FILES 
	"${CMAKE_BINARY_DIR}/rush-config.cmake"
	DESTINATION ${INSTALL_CMAKE_DIR}
	COMPONENT dev
)

install(EXPORT ${TARGET_EXPORT_NAME}
	DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev
)
