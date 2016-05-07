include_directories(
  ${CMAKE_SOURCE_DIR}/extern/gtest/include
)

function (rush_test TEST_NAME)
  #message("creating " ${TEST_NAME} " with " ${ARGN})
  set (SRC_LIST)
  foreach (SRC_FILE ${ARGN})
    LIST(APPEND SRC_LIST ${SRC_FILE})
  endforeach ()
  add_executable (${TEST_NAME} ${SRC_LIST})
  target_link_libraries (${TEST_NAME} gtest gtest_main base rush_transport rush_streams)
  add_test(NAME ${TEST_NAME} COMMAND $<TARGET_FILE:${TEST_NAME}>)
endfunction (rush_test)



include(${CMAKE_SOURCE_DIR}/base/config.cmake)
include(${CMAKE_SOURCE_DIR}/rush/config.cmake)

#rush_test( test_streams_startstop startstop_test.cpp )
#rush_test( test_streams_packing packing_test.cpp )
