#pragma once

#include <ros/ros.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/point_cloud2_iterator.h>
#include <message_filters/subscriber.h>
#include <message_filters/synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>

#define __APP_NAME__ "point_cloud_combiner"

namespace mst_object_localization
{

using policy_t = message_filters::sync_policies::ApproximateTime<sensor_msgs::PointCloud2,
                                                                 sensor_msgs::PointCloud2>;

class PointCloudCombiner
{
  public:
    /*!
     * Constructor.
     * @param t_node_handle the ROS node handle.
     */
    explicit PointCloudCombiner(ros::NodeHandle& t_node_handle);

  private:
    /*!
     * Reads and verifies the ROS parameters.
     * @return true if successful.
     */
    bool readParameters();

    /*!
     * The callback method for lanes.
     * @param point_cloud_parent first PointCloud2 message.
     * @param point_cloud_child second PointCloud2 message.
     */
    void combinerCallback(const sensor_msgs::PointCloud2ConstPtr& t_first_cloud_msg,
                        const sensor_msgs::PointCloud2ConstPtr& t_second_cloud_msg);

    //! ROS node handle.
    ros::NodeHandle& m_node_handle;

    //! ROS parent PC topic subscriber.
    message_filters::Subscriber<sensor_msgs::PointCloud2> m_first_cloud_sub;

    //! ROS child PC topic subscriber.
    message_filters::Subscriber<sensor_msgs::PointCloud2> m_second_cloud_sub;

    // Message Filter synchronizer.
    message_filters::Synchronizer<policy_t> m_synchronizer;

    //! ROS parent PC topic name to subscribe to.
    std::string m_first_cloud_topic;

    //! ROS child PC topic name to subscribe to.
    std::string m_second_cloud_topic;

    //! ROS filtered cloud publisher.
    ros::Publisher m_combined_pub;

    std::string m_combined_frame_id;
};
}  // namespace mst_object_localization