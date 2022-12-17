#include "cloud_projector_viz_cpp_pkg/projected_cloud_vis.hpp"
#include "ros/ros.h"
namespace projection
{
    ProjectedCloudViz::ProjectedCloudViz(ros::NodeHandle &t_node_handle_cloud)
        : m_node_handle(t_node_handle_cloud), m_image_transport(m_node_handle),
          m_is_cloud_received{false}, m_is_image_received{false}

    {
        if (!readParameters())
        {
            ROS_ERROR("Could not read parameters.");
            ros::requestShutdown();
        }

        ROS_INFO("Image topic: : %s", m_image_topic.c_str());

        m_point_cloud_sub = m_node_handle.subscribe<sensor_msgs::PointCloud2>(
            m_point_cloud_topic, 1, &ProjectedCloudViz::cloudCallback, this);

        m_image_sub = m_image_transport.subscribe(m_image_topic, 1, &ProjectedCloudViz::imageCallback, this);

        cv::namedWindow("Projected_Point_Cloud", cv::WINDOW_NORMAL);

        ROS_INFO("Successfully launched node.");

        ros::Rate rate(20);
        ros::Rate rate_waiting(1);
        while (ros::ok())
        {

            if (m_is_image_received)
            {

                if (m_is_cloud_received)
                {

                    drawProjectedPoints(m_cloud, cv::Scalar{0,127,255}, 5);
                }

                cv::Mat outImg;
                cv::resize(m_cv_ptr->image, outImg,
                           cv::Size(m_cv_ptr->image.cols * 0.5, m_cv_ptr->image.rows * 0.5),
                           0, 0, CV_INTER_LINEAR);
                cv::imshow("Projected_Point_Cloud", outImg);
                cv::waitKey(1);
            }

            else
            {
                
                ROS_INFO("IMAGE HAS NOT BEEN PROVIDEDED YET .");
                rate_waiting.sleep();
            }
            ros::spinOnce();
            rate.sleep() ;
        }
    }
    bool ProjectedCloudViz::readParameters()
    {
        if (!m_node_handle.getParam("projected_cloud_topic", m_point_cloud_topic))
        {
            return false;
        }
        if (!m_node_handle.getParam("image_topic", m_image_topic))
        {
            return false;
        }

        return true;
    }

    void ProjectedCloudViz::cloudCallback(const sensor_msgs::PointCloud2ConstPtr &t_point_cloud)
    {
        m_is_cloud_received = true;
        m_cloud = *t_point_cloud;
    }

    void ProjectedCloudViz::imageCallback(const sensor_msgs::ImageConstPtr &t_image)
    {
        m_is_image_received = true;
        m_cv_ptr = cv_bridge::toCvCopy(t_image, sensor_msgs::image_encodings::BGR8);
    }

    void ProjectedCloudViz::drawProjectedPoints(const sensor_msgs::PointCloud2 &t_cloud,
                                                const cv::Scalar &t_color, const int &t_thickness)
    {
        sensor_msgs::PointCloud2ConstIterator<float> cloud_x(t_cloud, "pX");
        sensor_msgs::PointCloud2ConstIterator<float> cloud_y(t_cloud, "pY");

        for (size_t i = 0; i < t_cloud.height * t_cloud.width; ++i, ++cloud_x, ++cloud_y)
        {
            cv::Point point{static_cast<int>(*cloud_x), static_cast<int>(*cloud_y)};
            cv::circle(m_cv_ptr->image, point, 1, t_color, t_thickness);
        }
    }

}