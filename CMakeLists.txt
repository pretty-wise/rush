cmake_minimum_required (VERSION 3.0)
project (rush)
set (Rush_VERSION_MAJOR 0)
set (Rush_VERSION_MINOR 1)
set (Rush_VERSION 0.1)

include (settings.cmake)

set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries.")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for binaries.")
set(INSTALL_INCLUDE_DIR include CACHE PATH "Installation directory for includes.")
set(INSTALL_CMAKE_DIR cmake CACHE PATH "Installation directory for cmake files.")

find_package(base REQUIRED)

include_directories (
	${CMAKE_SOURCE_DIR}/transport/include
	${BASE_INCLUDES}
)

#
# punch client
#
add_library (punch_client STATIC
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

target_include_directories(punch_client PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/transport/include>
	$<INSTALL_INTERFACE:include>
)

target_link_libraries(punch_client PUBLIC base)

#
# punch server
#
add_library (punch_server
	transport/include/rush/punch/server.h
	transport/src/punch/server/server.cpp
	transport/src/punch/server/punch_server.h
  transport/src/punch/server/punch_server.cpp
)	

target_include_directories (punch_server PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/transport/include>
	$<INSTALL_INTERFACE:include>
)

target_link_libraries (punch_server PRIVATE base)

#
# rush transport
#

add_library (rush
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

target_link_libraries (rush PRIVATE 
	base 
	punch_client
)

install(TARGETS rush EXPORT rush_targets
	LIBRARY DESTINATION ${INSTALL_LIB_DIR}
	ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
	RUNTIME DESTINATION ${INSTALL_BIN_DIR}
	INCLUDES DESTINATION ${INSTALL_INCLUDE_DIR}
)

install(
	DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/rush"
	DESTINATION ${INSTALL_INCLUDE_DIR}
	FILES_MATCHING PATTERN "*.*h"
)

install(EXPORT rush_targets
	FILE RushTargets.cmake
	NAMESPACE Rush::
	DESTINATION cmake
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file("../RushConfigVersion.cmake"
	VERSION ${Rush_VERSION}
	COMPATIBILITY SameMajorVersion
)

install(FILES "RushConfig.cmake" "RushConfigVersion.cmake"
	DESTINATION cmake
)
