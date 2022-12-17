#pragma once

#include <ros/ros.h>
#include <Eigen/Dense>
#include <vector>
#include <cmath>

#include <message_filters/subscriber.h>
#include <message_filters/synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>

#include <pcl_conversions/pcl_conversions.h>
#include <pcl/PCLPointCloud2.h>
#include <pcl_ros/transforms.h>
#include <pcl_ros/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/registration/ndt.h>

#include <tf/tf.h>
#include <tf2_ros/transform_listener.h>
#include <tf2/LinearMath/Matrix3x3.h>
#include <tf2_eigen/tf2_eigen.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>

#include "sensor_msgs/PointCloud2.h"
#include "sensor_msgs/PointCloud.h"
#include "sensor_msgs/point_cloud2_iterator.h"

#define __APP_NAME__ "ndt_calibrator"

namespace mst_object_localization
{
using policy_t = message_filters::sync_policies::ApproximateTime<sensor_msgs::PointCloud2,
                                                                 sensor_msgs::PointCloud2>;

class NdtCalibrator
{
  public:
    /*!
     * Constructor.
     * @param t_node_handle the ROS node handle.
     */
    explicit NdtCalibrator(ros::NodeHandle& t_node_handle);

  private:
    typedef pcl::PointXYZI PointT;

    /*!
     * Reads and verifies the ROS parameters.
     * @return true if successful.
     */
    bool readParameters();

    /*!
     * The callback method for message filter.
     * @param point_cloud_parent the parent PointCloud2 message.
     * @param point_cloud_child the child PointCloud2 message.
     */
    void messageFilterCallback(const sensor_msgs::PointCloud2ConstPtr& t_parent_cloud_msg,
                               const sensor_msgs::PointCloud2ConstPtr& t_child_cloud_msg);

    /*!
     * Applies a Voxel Grid filter to the point cloud.
     * @param t_in_cloud_ptr point cloud to downsample.
     * @param t_out_cloud_ptr downsampled point cloud.
     * @param t_voxel_size size of the voxel used to downsample the child point cloud.
     */
    void downSampleCloud(pcl::PointCloud<PointT>::Ptr t_in_cloud_ptr,
                         pcl::PointCloud<PointT>::Ptr t_out_cloud_ptr, double t_voxel_size);

    /*!
     * Check transform between lidars.
     * @param t_trans_par_to_child reference for TransformStamped message.
     * @ if there is static transforme between parent and child pointclouds, return true
     */
    bool checkTransform(geometry_msgs::TransformStamped& t_trans_par_to_child);

    /*!
     * If there is no transforme, this function calculate transforme between lidars.
     */
    void ndtAlgorithm();

    /*!
     * A function that calculate the rigid body transformation between two lidar.
     */
    void transform();

    /*!
     * A function that calculate the rigid body transformation between two lidar.
     */
    void quaternion_to_euler();

    /*!
     * Create a transforme matrix by initial guesses.
     */
    void initialGuess();

    /*!
     * Convert pcl pointcloud to ros message and publish.
     * @param t_cloud_to_publish pcl pointcloud which will publish.
     */
    void publishPointCloud(pcl::PointCloud<PointT>::ConstPtr t_pcl_cloud);

    void publishPointCloud(sensor_msgs::PointCloud2 t_sensor_msg);

    //! ROS node handle.
    ros::NodeHandle& m_node_handle;

    //! ROS parent PC topic subscriber.
    message_filters::Subscriber<sensor_msgs::PointCloud2> m_parent_cloud_sub;

    //! ROS child PC topic subscriber.
    message_filters::Subscriber<sensor_msgs::PointCloud2> m_child_cloud_sub;

    // Message Filter synchronizer.
    message_filters::Synchronizer<policy_t> m_synchronizer;

    //! ROS parent PC topic name to subscribe to.
    std::string m_parent_cloud_topic;

    //! ROS child PC topic name to subscribe to.
    std::string m_child_cloud_topic;

    //! ROS filtered cloud publisher.
    ros::Publisher m_combined_cloud_pub;

    //! Stores parent pointcloud ros message.
    sensor_msgs::PointCloud2ConstPtr m_parent_cloud_msg;

    //! Stores child pointcloud ros message.
    sensor_msgs::PointCloud2ConstPtr m_child_cloud_msg;

    //! Flag for receiving parent and child pointcloud.
    bool m_point_cloud_received = false;

    //! Stores the transforme produced by Ndt Algorithm.
    Eigen::Matrix4f m_current_guess;

    //! Stores the initial guess for Ndt Algorithm.
    Eigen::Matrix4f m_initial_guess;

    Eigen::Matrix4d m_transforme_mat;

    //! Tf2 stuff for extrinsic matrix.
    tf2_ros::Buffer tfBuffer;
    tf2_ros::TransformListener tfListener;

    geometry_msgs::TransformStamped m_trans_par_to_child;

    //! Frame ID of Parent Point Cloud.
    std::string m_parent_frame_id;

    //! Frame ID of Child Point Cloud.
    std::string m_child_frame_id;

    //! Size of voxel used to downsample the child point cloud.
    double m_voxel_size;

    //! The transformation epsilon in order for an optimization to be considered as having converged
    //! to the final solution.
    double m_ndt_epsilon;

    //! Set the newton line search maximum step length
    double m_ndt_step_size;

    //! Size of voxel used to downsample the parent point cloud.
    double m_ndt_resolution;

    //! The maximum number of iterations the initial optimization should run.
    int m_ndt_iterations;

    //! Initial guess for x.
    double m_in_x;

    //! Initial guess for y.
    double m_in_y;

    //! Initial guess for z.
    double m_in_z;

    //! Initial guess for roll.
    double m_in_roll;

    //! Initial guess for pitch.
    double m_in_pitch;

    //! Initial guess for yaw.
    double m_in_yaw;
};
}  // namespace mst_object_localization