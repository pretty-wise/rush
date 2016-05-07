include (${CMAKE_SOURCE_DIR}/base/config.cmake)
include (${CMAKE_SOURCE_DIR}/rush/config.cmake)

set (PUBLIC_FILES
  ../include/rush/streams.h
)

set (PRIVATE_FILES
	streams.cpp
)

add_library (rush_streams ${PUBLIC_FILES} ${PRIVATE_FILES}) 
target_link_libraries (rush_streams rush_transport base)
