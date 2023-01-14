#include "mst_object_localization/Visualize.hpp"

namespace mst_object_localization
{
Visualize::Visualize(ros::NodeHandle& t_node_handle_image, ros::NodeHandle& t_node_handle_cloud,
                     ros::NodeHandle& t_node_handle_filtered, ros::NodeHandle& t_node_handle_yolo)
  : m_node_handle_image(t_node_handle_image)
  , m_node_handle_cloud{ t_node_handle_cloud }
  , m_node_handle_filtered{ t_node_handle_filtered }
  , m_node_handle_yolo{ t_node_handle_yolo }
  , m_image_transport(m_node_handle_image)
{
    if (!readParameters())
    {
        ROS_ERROR("Could not read parameters.");
        ros::requestShutdown();
    }

    ROS_INFO("Image topic: %s", m_image_rect_topic.c_str());
    ROS_INFO("Point cloud topic: %s", m_cloud_projected_topic.c_str());

    ros::CallbackQueue callback_queue_cloud;
    m_node_handle_cloud.setCallbackQueue(&callback_queue_cloud);

    ros::CallbackQueue callback_queue_filtered;
    m_node_handle_filtered.setCallbackQueue(&callback_queue_filtered);

    ros::CallbackQueue callback_queue_yolo;
    m_node_handle_yolo.setCallbackQueue(&callback_queue_yolo);

    m_image_subscriber =
        m_image_transport.subscribe(m_image_rect_topic, 10, &Visualize::imageCallback, this);

    m_image_publisher = m_image_transport.advertise("visualize_calib", 1);

    m_cloud_subscriber = m_node_handle_cloud.subscribe<sensor_msgs::PointCloud2>(
        m_cloud_projected_topic, 1, &Visualize::projectedPointCloudCallback, this);

    m_filtered_subscriber =
        m_node_handle_filtered.subscribe<mst_object_localization::ArrayOfPointCloud2s>(
            m_cloud_filtered_topic, 1, &Visualize::filteredPointCloudCallback, this);

    m_yolo_subscriber = m_node_handle_yolo.subscribe<darknet_ros_msgs::BoundingBoxes>(
        m_yolo_topic, 1, &Visualize::yoloCloudCallback, this);

    std::thread spinner_thread_cloud([&callback_queue_cloud]() {
        ros::SingleThreadedSpinner spinner_cloud;
        spinner_cloud.spin(&callback_queue_cloud);
    });

    std::thread spinner_thread_filtered([&callback_queue_filtered]() {
        ros::SingleThreadedSpinner spinner_filtered;
        spinner_filtered.spin(&callback_queue_filtered);
    });

    std::thread spinner_thread_yolo([&callback_queue_yolo]() {
        ros::SingleThreadedSpinner spinner_yolo;
        spinner_yolo.spin(&callback_queue_yolo);
    });

    ROS_INFO("Successfully launched node.");

    ros::spin();
    spinner_thread_cloud.join();
}

bool Visualize::readParameters()
{
    if (!m_node_handle_image.getParam("image_rect_topic", m_image_rect_topic))
    {
        return false;
    }
    if (!m_node_handle_image.getParam("cloud_projected_topic", m_cloud_projected_topic))
    {
        return false;
    }
    if (!m_node_handle_image.getParam("cloud_filtered_topic", m_cloud_filtered_topic))
    {
        return false;
    }
    if (!m_node_handle_image.getParam("yolo_topic", m_yolo_topic))
    {
        return false;
    }
    if (!m_node_handle_image.getParam("extended_x_min", m_extended_x_min))
    {
        return false;
    }
    if (!m_node_handle_image.getParam("extended_x_max", m_extended_x_max))
    {
        return false;
    }
    if (!m_node_handle_image.getParam("extended_y_min", m_extended_y_min))
    {
        return false;
    }
    if (!m_node_handle_image.getParam("extended_y_max", m_extended_y_max))
    {
        return false;
    }
    return true;
}

void Visualize::imageCallback(const sensor_msgs::ImageConstPtr& t_image)
{
    ROS_INFO("IMAGE");
    // Convert ROS image message to OpenCV image via cv_bridge.
    m_cv_ptr = cv_bridge::toCvCopy(t_image, sensor_msgs::image_encodings::BGR8);
}

void Visualize::projectedPointCloudCallback(const sensor_msgs::PointCloud2ConstPtr& t_cloud)
{
    ROS_INFO("CLOUD");

    // Convert ROS PointCloud2 message to PCL type.
    pcl::PointCloud<pcl::PointXYZpXpY>::Ptr point_cloud(new pcl::PointCloud<pcl::PointXYZpXpY>);
    pcl::fromROSMsg(*t_cloud, *point_cloud);

    // for (auto& point : *point_cloud)
    // {
    //     cv::circle(m_cv_ptr->image, cv::Point{ point.pX, point.pY }, 1, cv::Scalar{ 0, 1000, 0 },
    //                4);
    // }

    // // cv::resize(m_cv_ptr->image, m_cv_ptr->image, cv::Size(), 0.4, 0.4); // Açma, hataveriyor.
    // cv::imshow("Image", m_cv_ptr->image);
    // cv::waitKey(1);
}

void Visualize::filteredPointCloudCallback(
    const mst_object_localization::ArrayOfPointCloud2sConstPtr& t_cloud)
{
    ROS_INFO("FILTERED");

    for (auto cloud : t_cloud->clouds)
    {
        // Convert ROS PointCloud2 message to PCL type.
        pcl::PointCloud<pcl::PointXYZpXpY>::Ptr point_cloud(new pcl::PointCloud<pcl::PointXYZpXpY>);
        pcl::fromROSMsg(cloud.cloud, *point_cloud);
        for (auto& point : *point_cloud)
        {
            cv::circle(m_cv_ptr->image, cv::Point{ point.pX, point.pY }, 1,
                       cv::Scalar{ 10000, 0, 0 }, 4, 10);
        }
    }
}

void Visualize::yoloCloudCallback(const darknet_ros_msgs::BoundingBoxesConstPtr& t_yolo_boxes)
{
    for (auto box : t_yolo_boxes->bounding_boxes)
    {
        cv::rectangle(m_cv_ptr->image,
                      cv::Point{ box.xmin - m_extended_x_min, box.ymin - m_extended_y_min },
                      cv::Point{ box.xmax + m_extended_x_max, box.ymax + m_extended_y_max },
                      cv::Scalar{ 0, 0, 1000 }, 15);
    }

    sensor_msgs::ImagePtr image_message =
        cv_bridge::CvImage(std_msgs::Header(), "bgr8", m_cv_ptr->image).toImageMsg();
    image_message->header.frame_id = "pylon_camera";
    image_message->header.stamp = ros::Time::now();

    m_image_publisher.publish(image_message);
}

}  // namespace mst_object_localization