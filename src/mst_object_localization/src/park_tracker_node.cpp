#include <ros/ros.h>
#include "mst_object_localization/ParkTracker.hpp"

int main(int argc, char** argv)
{
    ros::init(argc, argv, "park_tracker");
    ros::NodeHandle node_handle("~");

    mst_object_localization::ParkTracker park_tracker(node_handle);

    ros::spin();
    return 0;
}