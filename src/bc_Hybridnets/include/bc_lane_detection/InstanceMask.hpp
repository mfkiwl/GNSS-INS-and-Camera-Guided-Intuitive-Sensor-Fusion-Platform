#pragma once  // Daha önce include edilmişse birdaha include etme - header guard

#include <ros/ros.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/point_cloud2_iterator.h>
#include <std_msgs/Header.h>

#include <opencv2/opencv.hpp>
#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>

namespace bc_lane_detection
{
class InstanceMask
{
  public:
    /*!
     * Constructor.
     * @param t_node_handle the ROS node handle.
     */
    explicit InstanceMask(ros::NodeHandle& t_node_handle);  // tek parametrelik constructer için

  private:
    /*!
     * Reads and verifies the ROS parameters.
     * @return true if successful.
     */
    bool readParameters();

    /*!
     * Instance mask main callback method.
     * @param t_image the received Image message.
     */
    void imageCallback(const sensor_msgs::ImageConstPtr& t_image);

    /*!
     * Mask image by bgrPixel.
     * @return result mask
     */
    cv::Mat rangeFinder(const cv::Vec3b& bgrPixel,
                        const cv::InputArray& img);  // const ekleyince read-only oluyor, & koyunca
                                                     // copy almıyoruz direk kendisini alıyoruz,
                                                     // objenin üstünde değişiklik yapıyoruz.

    /*!
     * Instance mask main callback method.
     * @param t_image the received Image message.
     */
    void addToPointCloud(const cv::Mat& t_non_zero, const int& id,
                         sensor_msgs::PointCloud2Iterator<float>& t_x,
                         sensor_msgs::PointCloud2Iterator<float>& t_y,
                         sensor_msgs::PointCloud2Iterator<float>& t_c);

    //! ROS node handle.
    ros::NodeHandle& m_node_handle;

    //! Image topic name.
    std::string m_image_topic;

    //! Image weight.
    int m_image_weight;

    //! Image height.
    int m_image_height;

    //! Image transport object for dealing with images easily.
    image_transport::ImageTransport m_image_transport;

    //! Image subscriber.
    image_transport::Subscriber m_image_sub;

    //! Stores the image converted to OpenCV mat from sensor_msgs/Image
    cv_bridge::CvImagePtr m_cv_ptr;

    //! Point cloud publisher
    ros::Publisher m_point_cloud_pub;
};

}  // namespace bc_lane_detection