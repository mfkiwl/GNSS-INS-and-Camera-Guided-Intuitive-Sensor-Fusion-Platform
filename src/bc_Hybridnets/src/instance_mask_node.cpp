#include <ros/ros.h>
#include "bc_lane_detection/InstanceMask.hpp"

int main(int argc, char** argv)
{
    ros::init(argc, argv, "instance_mask");
    ros::NodeHandle node_handle("~");

    bc_lane_detection::InstanceMask mask(node_handle);

    ros::spin();
    return 0;
}