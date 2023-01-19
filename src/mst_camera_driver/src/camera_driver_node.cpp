#include <ros/ros.h>
#include "mst_camera_driver/CameraDriver.hpp"

int main(int argc, char** argv)
{
    ros::init(argc, argv, "camera_driver");
    ros::NodeHandle node_handle("~");

    mst_camera_driver::CameraDriver camera_driver(node_handle);

    ros::spin();
    return 0;
}