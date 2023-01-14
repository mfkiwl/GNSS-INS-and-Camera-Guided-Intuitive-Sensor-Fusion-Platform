#include <ros/ros.h>
#include "mst_object_localization/DarknetRepublish.hpp"

int main(int argc, char** argv)
{
    ros::init(argc, argv, "darknet_republish");
    ros::NodeHandle node_handle("~");

    mst_darknet_republish::DarknetRepublish darknet_republish(node_handle);

    ros::spin();
    return 0;
}