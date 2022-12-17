#pragma once

#include "ros/ros.h"
#include "sensor_msgs/PointCloud2.h"
#include "sensor_msgs/point_cloud2_iterator.h"
#include <std_msgs/Bool.h>
#include <math.h>

#define __APP_NAME__ "non_message_filter"

namespace mst_object_localization
{

    class NonMessageFilter

    {

    public:
        /*!
         * Constructor.
         * @param t_node_handle the ROS node handle.
         */
        NonMessageFilter(ros::NodeHandle &nodeHandle);

    private:
        /*!
         * Reads and verifies the ROS parameters.
         * @return true if successful.
         */
        bool readParameters();

        /*!
         * Raw point cloud callback method.
         * @param t_point_cloud the received PointCloud2 message.
         */
        void FirstCLoud(const sensor_msgs::PointCloud2::ConstPtr &t_point_cloud);
        void SecondCloud(const sensor_msgs::PointCloud2::ConstPtr &t_point_cloud);

        void CombinedCloud(const sensor_msgs::PointCloud2 &t_first_cloud, const sensor_msgs::PointCloud2 &t_secound_cloud);

        //! ROS node handle.
        ros::NodeHandle &m_node_handle;

        //! ROS m_pointcloud member.
        sensor_msgs::PointCloud2 m_first_cloud;

        //! ROS m_pointcloud_lane member.
        sensor_msgs::PointCloud2 m_second_cloud;

        //! ROS m_is_cloud_received member.
        bool first_cloud_is_recieved;

        //! ROS m_is_cloud_received member.
        bool second_cloud_is_recieved;

        //! ROS m_is_cloud_received member.
        ros::Publisher m_publisher;

        //! ROS m_is_cloud_received member.
        ros::Subscriber m_subscriber_second_cloud;

        //! Lidar topic name.
        ros::Subscriber m_subscriber_first_cloud;

        //! Lidar topic name.
        std::string m_topic_of_first_cloud;

        //! Lidar publisher topic name.
        std::string m_topic_of_second_cloud;

        //! Lidar publisher topic name.
        std::string m_topic_of_publisher;


    };

}