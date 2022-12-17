#pragma once

#include <ros/ros.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/point_cloud2_iterator.h>
#include <opencv2/opencv.hpp>
#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>

namespace projection
{
    class ProjectedCloudViz
    {
    public:
        explicit ProjectedCloudViz(ros::NodeHandle &t_node_handle);

    private:
        bool readParameters();

        void cloudCallback(const sensor_msgs::PointCloud2ConstPtr &t_point_cloud);

        void imageCallback(const sensor_msgs::ImageConstPtr &t_image);

        void drawProjectedPoints(const sensor_msgs::PointCloud2 &t_cloud, const cv::Scalar &t_color,
                                 const int &t_thickness);

        ros::NodeHandle &m_node_handle;

        std::string m_point_cloud_topic;

        std::string m_image_topic;

        ros::Subscriber m_point_cloud_sub;

        image_transport::ImageTransport m_image_transport;

        image_transport::Subscriber m_image_sub;

        sensor_msgs::PointCloud2 m_cloud;

        cv_bridge::CvImagePtr m_cv_ptr;

        bool m_is_cloud_received;

        bool m_is_image_received;
    };

}