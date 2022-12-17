#pragma once

#include "ros/ros.h"
#include "sensor_msgs/PointCloud2.h"
#include "sensor_msgs/point_cloud2_iterator.h"
#include "std_msgs/Bool.h"

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <map>

#define __APP_NAME__ "color_cluster"

namespace mst_object_localization
{

    class ColorCluster
    {

    public:
        /*!
         * Constructor.
         * @param t_node_handle the ROS node handle.
         */
        ColorCluster(ros::NodeHandle &nodeHandle);

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
        void lidarCallBack(const sensor_msgs::PointCloud2::ConstPtr &t_point_cloud);

        void simCluster(const sensor_msgs::PointCloud2 &t_point_cloud, std::map<std::string, float> &t_roi);

        void realCluster(const sensor_msgs::PointCloud2 &t_point_cloud, std::map<std::string, float> &t_roi);

        //! ROS node handle.
        ros::NodeHandle &m_node_handle;

        //! ROS m_pointcloud member.
        sensor_msgs::PointCloud2 m_point_cloud;

        //! ROS m_is_cloud_received member.
        ros::Publisher m_publisher;

        //! ROS m_is_cloud_received member.
        ros::Subscriber m_subscriber;

        //! Lidar topic name.
        std::string m_topic_of_subscriber;

        //! Lidar publisher topic name.
        std::string m_topic_of_publisher;

        //! Region of interest for left side.
        std::map<std::string, float> m_region_of_interest;

        //! sim intensity range
        std::map<std::string, float> m_intensity_sim_range;

        //! real intensity range
        std::map<std::string, float> m_intensity_real_range;

        //! The if data is received line .
        bool m_is_cloud_received;

        //!  distance x treshold.
        float m_detected_x_value;

        //!  distance y treshold.
        float m_detected_y_value;

        //!  distance z treshold.
        float m_detected_z_value;

        //!  enlarege the region of interest.
        bool m_bool_of_radius;

        //!  distance left_cloud treshold.
        double m_treshold_cloud;

        //!  whether lane points is detected or not.
        float m_elimineted_velodyne_point_x;
        
        //!  whether lane points is detected or not.
        float m_elimineted_velodyne_point_y;

        //!  whether lane points is detected or not.
        float m_elimineted_velodyne_point_z;

        //! is running in simulation mode.
        bool m_is_running_sim;


    };

}