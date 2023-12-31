#include <ros/ros.h>
#include <sensor_msgs/PointCloud2.h>
#include <pcl/point_cloud.h>
#include <pcl_conversions/pcl_conversions.h>

// #include <visualization_msgs/Marker.h>
// #include <visualization_msgs/MarkerArray.h>

#include <geometry_msgs/Point.h>

#include <kdtree.h>
#include <iostream>

int main(int argc, char** argv)
{
    ros::init(argc, argv, "Detection");
    ros::NodeHandle node_handle("~");

    Detection detection(argc, argv, node_handle);

    return 0;
}