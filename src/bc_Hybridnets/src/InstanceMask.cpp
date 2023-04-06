#include "bc_lane_detection/InstanceMask.hpp"

namespace bc_lane_detection
{
InstanceMask::InstanceMask(ros::NodeHandle& t_node_handle)
  : m_node_handle(t_node_handle), m_image_transport(m_node_handle)
{
    if (!readParameters())
    {
        ROS_ERROR("Could not read parameters.");
        ros::requestShutdown();
    }

    ROS_INFO("Image topic: : %s", m_image_topic.c_str());
    ROS_INFO("Image weight: : %i", m_image_weight);
    ROS_INFO("Image height: : %i", m_image_height);

    m_image_sub = m_image_transport.subscribe(m_image_topic, 1, &InstanceMask::imageCallback, this);

    m_point_cloud_pub = m_node_handle.advertise<sensor_msgs::PointCloud2>("lane2d", 1);

    ROS_INFO("Successfully launched Instance Mask node.");
}

bool InstanceMask::readParameters()
{
    if (!m_node_handle.getParam("image_topic", m_image_topic))
    {
        return false;
    }
    else if (!m_node_handle.getParam("image_weight", m_image_weight))
    {
        return false;
    }
    else if (!m_node_handle.getParam("image_height", m_image_height))
    {
        return false;
    }
    return true;
}

void InstanceMask::imageCallback(const sensor_msgs::ImageConstPtr& t_image)
{
    m_cv_ptr = cv_bridge::toCvCopy(t_image, sensor_msgs::image_encodings::BGR8);

    // -------------------

    cv::resize(m_cv_ptr->image, m_cv_ptr->image, cv::Size(m_image_weight, m_image_height),
               cv::INTER_LINEAR);

    cv::Vec3b greenbgrPixel(0, 250, 0);
    cv::Vec3b redbgrPixel(0, 0, 250);
    cv::Vec3b bluebgrPixel(250, 0, 0);
    cv::Vec3b turquoisebgrPixel(250, 250, 0);
    cv::Vec3b yellowbgrPixel(0, 250, 250);

    cv::Mat greenresultBGR = InstanceMask::rangeFinder(greenbgrPixel, m_cv_ptr->image);
    cv::Mat redresultBGR = InstanceMask::rangeFinder(redbgrPixel, m_cv_ptr->image);
    cv::Mat blueresultBGR = InstanceMask::rangeFinder(bluebgrPixel, m_cv_ptr->image);
    cv::Mat turquoiseresultBGR = InstanceMask::rangeFinder(turquoisebgrPixel, m_cv_ptr->image);
    cv::Mat yellowresultBGR = InstanceMask::rangeFinder(yellowbgrPixel, m_cv_ptr->image);

    // cv::imshow("greenresultBGR", greenresultBGR);
    // cv::imshow("redresultBGR", redresultBGR);
    // cv::imshow("blueresultBGR", blueresultBGR);
    // cv::imshow("turquoiseresultBGR", turquoiseresultBGR);
    // cv::imshow("yellowresultBGR", yellowresultBGR);
    // cv::waitKey(1);

    // Convert images 3 channel image to 1 channel image for using cv::findNonZero
    cv::cvtColor(greenresultBGR, greenresultBGR, cv::COLOR_BGR2GRAY);
    cv::cvtColor(redresultBGR, redresultBGR, cv::COLOR_BGR2GRAY);
    cv::cvtColor(blueresultBGR, blueresultBGR, cv::COLOR_BGR2GRAY);
    cv::cvtColor(turquoiseresultBGR, turquoiseresultBGR, cv::COLOR_BGR2GRAY);
    cv::cvtColor(yellowresultBGR, yellowresultBGR, cv::COLOR_BGR2GRAY);

    cv::Mat greenNonZero, redNonZero, blueNonZero, turquoiseNonZero, yellowNonZero;

    // Find non-zero pixels
    cv::findNonZero(greenresultBGR, greenNonZero);
    cv::findNonZero(redresultBGR, redNonZero);
    cv::findNonZero(blueresultBGR, blueNonZero);
    cv::findNonZero(turquoiseresultBGR, turquoiseNonZero);
    cv::findNonZero(yellowresultBGR, yellowNonZero);

    // Calculate non-zero matrix size to determine point cloud size
    const size_t green_size = greenNonZero.size().width * greenNonZero.size().height;
    const size_t red_size = redNonZero.size().width * redNonZero.size().height;
    const size_t blue_size = blueNonZero.size().width * blueNonZero.size().height;
    const size_t turquoise_size = turquoiseNonZero.size().width * turquoiseNonZero.size().height;
    const size_t yellow_size = yellowNonZero.size().width * yellowNonZero.size().height;

    // calculate point cloud size
    const size_t sum = green_size + red_size + blue_size + turquoise_size + yellow_size;

    sensor_msgs::PointCloud2 point_cloud;
    sensor_msgs::PointCloud2Modifier modifier(point_cloud);
    modifier.resize(sum);

    modifier.setPointCloud2Fields(3, "x", 1, sensor_msgs::PointField::FLOAT32, "y", 1,
                                  sensor_msgs::PointField::FLOAT32, "c", 1,
                                  sensor_msgs::PointField::FLOAT32);

    sensor_msgs::PointCloud2Iterator<float> point_cloud_x(point_cloud, "x");
    sensor_msgs::PointCloud2Iterator<float> point_cloud_y(point_cloud, "y");
    sensor_msgs::PointCloud2Iterator<float> point_cloud_c(point_cloud, "c");

    addToPointCloud(greenNonZero, 1, point_cloud_x, point_cloud_y, point_cloud_c);
    addToPointCloud(redNonZero, 2, point_cloud_x, point_cloud_y, point_cloud_c);
    addToPointCloud(blueNonZero, 3, point_cloud_x, point_cloud_y, point_cloud_c);
    addToPointCloud(turquoiseNonZero, 4, point_cloud_x, point_cloud_y, point_cloud_c);
    addToPointCloud(yellowNonZero, 5, point_cloud_x, point_cloud_y, point_cloud_c);

    std_msgs::Header header;
    header.frame_id = "camera";
    header.stamp = ros::Time::now();

    point_cloud.header = header;

    m_point_cloud_pub.publish(point_cloud);

    // -------------------
}

cv::Mat InstanceMask::rangeFinder(const cv::Vec3b& bgrPixel, const cv::InputArray& img)
{
    int thresh = 5;

    cv::Mat3b bgr(bgrPixel);

    cv::Scalar maxBGR =
        cv::Scalar(bgrPixel.val[0] + thresh, bgrPixel.val[1] + thresh, bgrPixel.val[2] + thresh);
    cv::Scalar minBGR =
        cv::Scalar(bgrPixel.val[0] - thresh, bgrPixel.val[1] - thresh, bgrPixel.val[2] - thresh);

    cv::Mat maskBGR, resultBGR;
    cv::inRange(img, minBGR, maxBGR, maskBGR);
    cv::bitwise_and(img, img, resultBGR, maskBGR);

    return resultBGR;
}

void InstanceMask::addToPointCloud(const cv::Mat& t_non_zero, const int& id,
                                   sensor_msgs::PointCloud2Iterator<float>& t_x,
                                   sensor_msgs::PointCloud2Iterator<float>& t_y,
                                   sensor_msgs::PointCloud2Iterator<float>& t_c)
{
    size_t size = t_non_zero.size().width * t_non_zero.size().height;

    for (size_t i = 0; i < size; ++i, ++t_x, ++t_y, ++t_c)
    {
        const cv::Point point = t_non_zero.at<cv::Point>(1, i);

        *t_x = point.x;
        *t_y = point.y;
        *t_c = id;
    }
}

}  // namespace bc_lane_detection