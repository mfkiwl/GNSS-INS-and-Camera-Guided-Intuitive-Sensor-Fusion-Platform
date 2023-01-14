#pragma once

#include <thread>

#include <ros/ros.h>
#include <ros/callback_queue.h>

#include <opencv2/opencv.hpp>
#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>

#include <pcl/point_cloud.h>
#include <pcl_ros/point_cloud.h>
#include "mst_object_localization/PointXYZpXpY.hpp"

#include <sensor_msgs/Image.h>
#include <sensor_msgs/PointCloud2.h>

#include <mst_object_localization/ArrayOfPointCloud2s.h>
#include <darknet_ros_msgs/BoundingBoxes.h>

namespace mst_object_localization
{
class Visualize
{
  public:
    /*!
     * Constructor.
     * @param t_node_handle_image the ROS node handle for image thread.
     * @param t_node_handle_cloud the ROS node handle for point cloud thread.
     * @param t_node_handle_filtered the ROS node handle for filtered point cloud thread.
     * @param t_node_handle_yolo the ROS node handle for filtered point cloud thread.
     */
    Visualize(ros::NodeHandle& t_node_handle_image, ros::NodeHandle& t_node_handle_cloud,
              ros::NodeHandle& t_node_handle_filtered, ros::NodeHandle& t_node_handle_yolo);

  private:
    /*!
     * Reads and verifies the ROS parameters.
     * @return true if successful.
     */
    bool readParameters();

    /*!
     * Undistorted image callback method.
     * @param t_image the received Image message.
     */
    void imageCallback(const sensor_msgs::ImageConstPtr& t_image);

    /*!
     * Projected point cloud callback method.
     * @param t_cloud the received PointCloud2 message.
     */
    void projectedPointCloudCallback(const sensor_msgs::PointCloud2ConstPtr& t_cloud);

    /*!
     * Filtered point cloud callback method.
     * @param t_cloud the received PointCloud2 message.
     */
    void filteredPointCloudCallback(const mst_object_localization::ArrayOfPointCloud2sConstPtr& t_cloud);

    /*!
     * YOLO bounding boxes callback method.
     * @param t_yolo_boxes the received BoundingBoxes message.
     */
    void yoloCloudCallback(const darknet_ros_msgs::BoundingBoxesConstPtr& t_yolo_boxes);

    //! ROS node handle for image thread.
    ros::NodeHandle& m_node_handle_image;

    //! ROS node handle for point cloud thread.
    ros::NodeHandle& m_node_handle_cloud;

    //! ROS node handle for filtered point cloud thread.
    ros::NodeHandle& m_node_handle_filtered;

    //! ROS node handle for filtered yolo thread.
    ros::NodeHandle& m_node_handle_yolo;

    //! Undistorted image topic name.
    std::string m_image_rect_topic;

    //! Projected point cloud topic name.
    std::string m_cloud_projected_topic;

    //! Filtered point cloud topic name.
    std::string m_cloud_filtered_topic;

    //! YOLO topic name.
    std::string m_yolo_topic;

    //! Image transport object for dealing with images easily.
    image_transport::ImageTransport m_image_transport;

    //! Undistorted image subscriber.
    image_transport::Subscriber m_image_subscriber;

    //! Undistorted image publisher.
    image_transport::Publisher m_image_publisher;

    //! Stores the image converted to OpenCV mat from sensor_msgs/Image
    cv_bridge::CvImagePtr m_cv_ptr;

    //! Projected point cloud subscriber.
    ros::Subscriber m_cloud_subscriber;

    //! Filtered point cloud subscriber.
    ros::Subscriber m_filtered_subscriber;

    //! YOLO subscriber.
    ros::Subscriber m_yolo_subscriber;

    int m_extended_x_min;
    int m_extended_x_max;
    int m_extended_y_min;
    int m_extended_y_max;
};

}  // namespace mst_object_localization
