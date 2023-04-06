#include <ros/ros.h>
#include "bc_lane_detection/VisualizeLane.hpp"

int main(int argc, char** argv)
{
    ros::init(argc, argv, "viz_lane2d");
    ros::NodeHandle node_handle("~");

    bc_lane_detection::VisualizeLane viz(node_handle);

    return 0;
}