#include "cloud_projector_viz_cpp_pkg/projected_cloud_vis.hpp"
#include "ros/ros.h"

int main(int argc, char **argv)
{
    ros::init(argc, argv, "cloud_projector_vis");
    ros::NodeHandle nodeHandle("~");

    projection::ProjectedCloudViz ProjectedCloudViz(nodeHandle);

    ros::spin();
    return 0;
}
