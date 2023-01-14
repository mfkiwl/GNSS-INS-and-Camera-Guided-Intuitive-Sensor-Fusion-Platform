#include <ros/ros.h>
#include "mst_object_localization/Visualize.hpp"

int main(int argc, char** argv)
{
    ros::init(argc, argv, "cloud_painter");
    ros::NodeHandle node_handle_image("~");
    ros::NodeHandle node_handle_cloud("~");
    ros::NodeHandle node_handle_filtered("~");
    ros::NodeHandle node_handle_yolo("~");

    mst_object_localization::Visualize visualize(node_handle_image, node_handle_cloud, node_handle_filtered, node_handle_yolo);

    return 0;
}