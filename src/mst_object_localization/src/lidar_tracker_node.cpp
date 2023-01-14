#include <ros/ros.h>
#include "mst_object_localization/LidarTracker.hpp"

int main(int argc, char** argv)
{
    ros::init(argc, argv, "lidar_tracker");
    ros::NodeHandle node_handle("~");

    mst_object_localization::LidarTracker lidar_tracker(node_handle);

    ros::spin();
    return 0;
}