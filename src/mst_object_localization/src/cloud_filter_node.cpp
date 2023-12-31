#include <ros/ros.h>
#include "mst_object_localization/CloudFilter.hpp"

int main(int argc, char** argv)
{
    ros::init(argc, argv, "cloud_painter");
    ros::NodeHandle node_handle("~");

    mst_object_localization::CloudFilter cloud_filter(node_handle);

    ros::spin();
    return 0;
}