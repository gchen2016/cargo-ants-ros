cmake_minimum_required (VERSION 2.6)
if (COMMAND cmake_policy)
  cmake_policy (SET CMP0003 NEW)
  cmake_policy (SET CMP0005 NEW)
  if (POLICY CMP0011)
    cmake_policy (SET CMP0011 NEW)
  endif (POLICY CMP0011)
  if (POLICY CMP0042)
    cmake_policy (SET CMP0042 NEW)
  endif (POLICY CMP0042)
endif (COMMAND cmake_policy)

project (cargo_ants_udp)
set (CMAKE_VERBOSE_MAKEFILE ON)

find_package (catkin REQUIRED COMPONENTS
  cargo_ants_msgs
  roscpp
)

catkin_package (
  INCLUDE_DIRS include
  LIBRARIES cargo_ants_udp
  CATKIN_DEPENDS cargo_ants_msgs roscpp
  #  DEPENDS system_lib
)

include_directories (
  include
  ${catkin_INCLUDE_DIRS}
  )

add_library (cargo_ants_udp SHARED
  src/UDPMessage.cpp
  src/UDPClient.cpp
  src/UDPServer.cpp
  )
#target_link_libraries ()

add_executable (test_udp src/test_udp.cpp)
target_link_libraries (test_udp cargo_ants_udp)
add_dependencies(test_udp ${catkin_EXPORTED_TARGETS})

add_executable (test_feed_udp2ros src/test_feed_udp2ros.cpp)
target_link_libraries (test_feed_udp2ros cargo_ants_udp)
add_dependencies(test_feed_udp2ros ${catkin_EXPORTED_TARGETS})

add_executable (test_node_udp2ros src/test_node_udp2ros.cpp)
target_link_libraries (test_node_udp2ros cargo_ants_udp ${catkin_LIBRARIES})
add_dependencies(test_node_udp2ros ${catkin_EXPORTED_TARGETS})

add_executable (test_node_ros2udp src/test_node_ros2udp.cpp)
target_link_libraries (test_node_ros2udp cargo_ants_udp ${catkin_LIBRARIES})
add_dependencies(test_node_ros2udp ${catkin_EXPORTED_TARGETS})
