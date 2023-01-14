cmake_minimum_required(VERSION 3.0.2)
project(mst_object_localization)

## Compile as C++11, supported in ROS Kinetic and newer
# add_compile_options(-std=c++11)
add_compile_options(-Wall -Wextra)

## Use C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

## By adding -Wall and -Werror, the compiler does not ignore warnings anymore,
## enforcing cleaner code.
# add_definitions(-Wall -Werror)

set(OpenCV_DIR /usr/lib/x86_64-linux-gnu/cmake/opencv4)

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
find_package(catkin REQUIRED COMPONENTS
  pcl_conversions
  pcl_ros
  roscpp
  rospy
  sensor_msgs
  std_msgs
  darknet_ros_msgs
  OpenCV
  cv_bridge
  message_generation
  actionlib_msgs
  image_transport
  # mst_camera_driver
  visualization_msgs
  actionlib
  yolov5_ros_msgs
)

## System dependencies are found with CMake's conventions
# find_package(Boost REQUIRED COMPONENTS system)


## Uncomment this if the package has a setup.py. This macro ensures
## modules and global scripts declared therein get installed
## See http://ros.org/doc/api/catkin/html/user_guide/setup_dot_py.html
# catkin_python_setup()

################################################
## Declare ROS messages, services and actions ##
################################################

## To declare and build messages, services or actions from within this
## package, follow these steps:
## * Let MSG_DEP_SET be the set of packages whose message types you use in
##   your messages/services/actions (e.g. std_msgs, actionlib_msgs, ...).
## * In the file package.xml:
##   * add a build_depend tag for "message_generation"
##   * add a build_depend and a exec_depend tag for each package in MSG_DEP_SET
##   * If MSG_DEP_SET isn't empty the following dependency has been pulled in
##     but can be declared for certainty nonetheless:
##     * add a exec_depend tag for "message_runtime"
## * In this file (CMakeLists.txt):
##   * add "message_generation" and every package in MSG_DEP_SET to
##     find_package(catkin REQUIRED COMPONENTS ...)
##   * add "message_runtime" and every package in MSG_DEP_SET to
##     catkin_package(CATKIN_DEPENDS ...)
##   * uncomment the add_*_files sections below as needed
##     and list every .msg/.srv/.action file to be processed
##   * uncomment the generate_messages entry below
##   * add every package in MSG_DEP_SET to generate_messages(DEPENDENCIES ...)

## Generate messages in the 'msg' folder
add_message_files(
  FILES
  PointCloud2WithId.msg
  ArrayOfPointCloud2s.msg
#   Message1.msg
#   Message2.msg
)

## Generate services in the 'srv' folder
# add_service_files(
#   FILES
#   Service1.srv
#   Service2.srv
# )

## Generate actions in the 'action' folder
# add_action_files(
#   FILES
#   Action1.action
#   Action2.action
# )

add_action_files(
  DIRECTORY action
  FILES ProjectedCloud.action
  Park.action
  SignTracker.action
  DeleteSign.action
)

## Generate added messages and services with any dependencies listed here
generate_messages(
  DEPENDENCIES
  std_msgs
  sensor_msgs
  visualization_msgs
  actionlib_msgs
)

################################################
## Declare ROS dynamic reconfigure parameters ##
################################################

## To declare and build dynamic reconfigure parameters within this
## package, follow these steps:
## * In the file package.xml:
##   * add a build_depend and a exec_depend tag for "dynamic_reconfigure"
## * In this file (CMakeLists.txt):
##   * add "dynamic_reconfigure" to
##     find_package(catkin REQUIRED COMPONENTS ...)
##   * uncomment the "generate_dynamic_reconfigure_options" section below
##     and list every .cfg file to be processed

## Generate dynamic reconfigure parameters in the 'cfg' folder
# generate_dynamic_reconfigure_options(
#   cfg/DynReconf1.cfg
#   cfg/DynReconf2.cfg
# )

###################################
## catkin specific configuration ##
###################################
## The catkin_package macro generates cmake config files for your package
## Declare things to be passed to dependent projects
## INCLUDE_DIRS: uncomment this if your package contains header files
## LIBRARIES: libraries you create in this project that dependent projects also need
## CATKIN_DEPENDS: catkin_packages dependent projects also need
## DEPENDS: system dependencies of this project that dependent projects also need
catkin_package(
   INCLUDE_DIRS include
#  LIBRARIES mst_object_localization
#  CATKIN_DEPENDS pcl_conversions pcl_ros roscpp rospy sensor_msgs std_msgs
  CATKIN_DEPENDS message_runtime
  actionlib_msgs
#  DEPENDS system_lib
)

###########
## Build ##
###########

## Specify additional locations of header files
## Your package locations should be listed before other locations
include_directories(
  include
  ${catkin_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
)

## Declare a C++ library
# add_library(${PROJECT_NAME}
#   src/${PROJECT_NAME}/mst_object_localization.cpp
# )

## Add cmake target dependencies of the library
## as an example, code may need to be generated before libraries
## either from message generation or dynamic reconfigure
# add_dependencies(${PROJECT_NAME} ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

## Declare a C++ executable
## With catkin_make all packages are built within a single CMake context
## The recommended prefix ensures that target names across packages don't collide
# add_executable(${PROJECT_NAME}_node src/mst_object_localization_node.cpp)

### CloudPainter
## Declare a cpp library
add_library(cloud_painter_core
   src/CloudPainter/CloudPainter.cpp
)
target_compile_features(cloud_painter_core INTERFACE cxx_std_11)

## Declare cpp executables
add_executable(cloud_painter
  src/CloudPainter/cloud_painter_node.cpp
  src/CloudPainter/CloudPainter.cpp
)
target_compile_features(cloud_painter INTERFACE cxx_std_11)

## Add dependencies to exported targets, like ROS msgs or srvs
add_dependencies(cloud_painter_core
  ${catkin_EXPORTED_TARGETS}
)

add_dependencies(cloud_painter
  ${catkin_EXPORTED_TARGETS}
)

## Specify libraries to link executable targets against
target_link_libraries(cloud_painter_core
  ${catkin_LIBRARIES}
)

target_link_libraries(cloud_painter
  cloud_painter_core
  ${catkin_LIBRARIES}
)



### CloudFilter
## Declare a cpp library
add_library(cloud_filter_core
   src/CloudFilter.cpp
)
target_compile_features(cloud_filter_core INTERFACE cxx_std_11)

## Declare cpp executables
add_executable(cloud_filter
  src/cloud_filter_node.cpp
  src/CloudFilter.cpp
)
target_compile_features(cloud_filter INTERFACE cxx_std_11)

## Add dependencies to exported targets, like ROS msgs or srvs
add_dependencies(cloud_filter_core
  ${catkin_EXPORTED_TARGETS}
)

add_dependencies(cloud_filter
  ${catkin_EXPORTED_TARGETS}
)

## Specify libraries to link executable targets against
target_link_libraries(cloud_filter_core
  ${catkin_LIBRARIES}
)

target_link_libraries(cloud_filter
  cloud_filter_core
  ${catkin_LIBRARIES}
)



### Visualize
## Declare cpp executables
# add_executable(visualize
#   src/visualize_node.cpp
# )
# target_compile_features(visualize INTERFACE cxx_std_11)

# add_dependencies(visualize
#   ${catkin_EXPORTED_TARGETS}
# )

# target_link_libraries(visualize
#   ${catkin_LIBRARIES}
# )



### ParkTracker
## Declare a cpp library
# add_library(park_tracker_core
#    src/ParkTracker.cpp
# )
# target_compile_features(park_tracker_core INTERFACE cxx_std_11)

# ## Declare cpp executables
# add_executable(park_tracker
#   src/park_tracker_node.cpp
#   src/ParkTracker.cpp
# )
# target_compile_features(park_tracker INTERFACE cxx_std_11)

# ## Add dependencies to exported targets, like ROS msgs or srvs
# add_dependencies(park_tracker_core
#   ${catkin_EXPORTED_TARGETS}
# )

# add_dependencies(park_tracker
#   ${catkin_EXPORTED_TARGETS}
# )

# ## Specify libraries to link executable targets against
# target_link_libraries(park_tracker_core
#   ${catkin_LIBRARIES}
# )

# target_link_libraries(park_tracker
#   park_tracker_core
#   ${catkin_LIBRARIES}
# )





### LidarTracker
## Declare a cpp library
# add_library(lidar_tracker_core
#    src/LidarTracker.cpp
# )
# target_compile_features(lidar_tracker_core INTERFACE cxx_std_11)

# ## Declare cpp executables
# add_executable(lidar_tracker
#   src/lidar_tracker_node.cpp
#   src/LidarTracker.cpp
# )
# target_compile_features(lidar_tracker INTERFACE cxx_std_11)

# ## Add dependencies to exported targets, like ROS msgs or srvs
# add_dependencies(lidar_tracker_core
#   ${catkin_EXPORTED_TARGETS}
# )

# add_dependencies(lidar_tracker
#   ${catkin_EXPORTED_TARGETS}
# )

# ## Specify libraries to link executable targets against
# target_link_libraries(lidar_tracker_core
#   ${catkin_LIBRARIES}
# )

# target_link_libraries(lidar_tracker
#   lidar_tracker_core
#   ${catkin_LIBRARIES}
# )




### DarknetRepublish
## Declare a cpp library
# add_library(darknet_republish_core
#    src/DarknetRepublish.cpp
# )
# target_compile_features(darknet_republish_core INTERFACE cxx_std_11)

# ## Declare cpp executables
# add_executable(darknet_republish
#   src/darknet_republish_node.cpp
#   src/DarknetRepublish.cpp
# )
# target_compile_features(darknet_republish INTERFACE cxx_std_11)

# ## Add dependencies to exported targets, like ROS msgs or srvs
# add_dependencies(darknet_republish_core
#   ${catkin_EXPORTED_TARGETS}
# )

# add_dependencies(darknet_republish
#   ${catkin_EXPORTED_TARGETS}
# )

# ## Specify libraries to link executable targets against
# target_link_libraries(darknet_republish_core
#   ${catkin_LIBRARIES}
# )

# target_link_libraries(darknet_republish
#   darknet_republish_core
#   ${catkin_LIBRARIES}
# )



### DarknetRepublish
## Declare cpp executables
# add_executable(cloud_filter_mf
#   src/cloud_filter_node_mf.cpp
# )
# target_compile_features(cloud_filter_mf INTERFACE cxx_std_11)

# add_dependencies(cloud_filter_mf
#   ${catkin_EXPORTED_TARGETS}
# )

# target_link_libraries(cloud_filter_mf
#   ${catkin_LIBRARIES}
# )



## Rename C++ executable without prefix
## The above recommended prefix causes long target names, the following renames the
## target back to the shorter version for ease of user use
## e.g. "rosrun someones_pkg node" instead of "rosrun someones_pkg someones_pkg_node"
# set_target_properties(${PROJECT_NAME}_node PROPERTIES OUTPUT_NAME node PREFIX "")

## Add cmake target dependencies of the executable
## same as for the library above
# add_dependencies(${PROJECT_NAME}_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

## Specify libraries to link a library or executable target against
# target_link_libraries(${PROJECT_NAME}_node
#   ${catkin_LIBRARIES}
# )

#############
## Install ##
#############

# all install targets should use catkin DESTINATION variables
# See http://ros.org/doc/api/catkin/html/adv_user_guide/variables.html

## Mark executable scripts (Python etc.) for installation
## in contrast to setup.py, you can choose the destination
# catkin_install_python(PROGRAMS
#   scripts/my_python_script
#   DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
# )

## Mark executables for installation
## See http://docs.ros.org/melodic/api/catkin/html/howto/format1/building_executables.html
# install(TARGETS ${PROJECT_NAME}_node
#   RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
# )

## Mark libraries for installation
## See http://docs.ros.org/melodic/api/catkin/html/howto/format1/building_libraries.html
# install(TARGETS ${PROJECT_NAME}
#   ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
#   LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
#   RUNTIME DESTINATION ${CATKIN_GLOBAL_BIN_DESTINATION}
# )

## Mark cpp header files for installation
# install(DIRECTORY include/${PROJECT_NAME}/
#   DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
#   FILES_MATCHING PATTERN "*.h"
#   PATTERN ".svn" EXCLUDE
# )

## Mark other files for installation (e.g. launch and bag files, etc.)
# install(FILES
#   # myfile1
#   # myfile2
#   DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
# )

#############
## Testing ##
#############

## Add gtest based cpp test target and link libraries
# catkin_add_gtest(${PROJECT_NAME}-test test/test_mst_object_localization.cpp)
# if(TARGET ${PROJECT_NAME}-test)
#   target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME})
# endif()

## Add folders to be run by python nosetests
# catkin_add_nosetests(test)

# add_library(kdtreeLibrary src/kdtree.cpp)

# add_executable(kdtree_main src/kdtree_main.cpp)
# add_dependencies(kdtree_main ${catkin_EXPORTED_TARGETS})
# target_link_libraries(kdtree_main kdtreeLibrary ${catkin_LIBRARIES})