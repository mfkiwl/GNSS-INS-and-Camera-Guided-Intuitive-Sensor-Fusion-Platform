#pragma once

#include <ros/ros.h>

#include <std_msgs/Bool.h>
#include <sensor_msgs/PointCloud2.h>
#include <visualization_msgs/Marker.h>
#include <visualization_msgs/MarkerArray.h>

#include <actionlib/client/simple_action_client.h>
#include <actionlib/client/terminal_state.h>
#include <actionlib/server/simple_action_server.h>

#include <mst_object_localization/SignTrackerAction.h>
#include <mst_object_localization/ParkAction.h>
#include <mst_object_localization/DeleteSignAction.h>

#include <pcl/point_cloud.h>
#include <pcl/filters/crop_box.h>
#include <pcl_conversions/pcl_conversions.h>

namespace mst_object_localization
{
// TODO: This class may be replaced with visualization_msgs::Marker.
class Sign
{
  public:
    double x;
    double y;
    double z;
    string Class;
};

class LidarTracker
{
  public:
    /*!
     * Constructor.
     * @param t_node_handle the ROS node handle.
     */
    LidarTracker(ros::NodeHandle& t_node_handle);

  private:
    /*!
     * Reads and verifies the ROS parameters.
     * @return true if successful.
     */
    bool readParameters();

    /*!
     * Sign markers callback method.
     * @param t_marker_array the received marker array message.
     */
    void signCallback(const visualization_msgs::MarkerArrayConstPtr& t_marker_array);

    /*!
     * Projected point cloud callback method.
     * @param t_point_cloud the received PointCloud2 message.
     */
    void cloudCallback(const sensor_msgs::PointCloud2ConstPtr& t_point_cloud);

    /*!
     * Red light delete callback method.
     * @param t_is_delete the received Bool message.
     */
    void noRedLightCallback(const std_msgs::BoolConstPtr& t_is_delete);

    /*!
     * Finish line check callback method.
     * @param t_is_finished the received finish bool message.
     */
    void finishCallback(const std_msgs::BoolConstPtr& t_is_finished);

    /*!
     * Prints "m_signs" vector.
     */
    void printActiveSigns();

    /*!
     * Calculate the distance between lidar and traffic sign.
     * @param t_x_coordinate the x coordinate of the sign.
     * @param t_y_coordinate the y coordinate of the sign.
     * @param t_z_coordinate the z coordinate of the sign.
     */
    double calculateDistance(const double& t_x_coordinate, const double& t_y_coordinate,
                             const double& t_z_coordinate);

    /*!
     * Search for the sign in the calculated area.
     * @param t_x_coordinate the x coordinate of the sign.
     * @param t_y_coordinate the y coordinate of the sign.
     * @param t_z_coordinate the z coordinate of the sign.
     */
    void trackSign(const double& t_x_coordinate, const double& t_y_coordinate,
                   const double& t_z_coordinate);

    /*!
     * Action server callback method to delete signs.
     * @param goal action goal.
     */
    void actionServerCallback(const mst_object_localization::DeleteSignGoalConstPtr& goal);

    /*!
     * Sign stop callback method.
     * @param t_is_stopped the received finish bool message.
     */
    void signStopCallback(const std_msgs::BoolConstPtr& t_is_stopped);

    /*!
     * Autonomous mode callback method.
     * @param t_is_autonomous the received finish bool message.
     */
    void autonomousCallback(const std_msgs::BoolConstPtr& t_is_autonomous);

    //! ROS node handle.
    ros::NodeHandle& m_node_handle;

    //! Action client for clustering
    actionlib::SimpleActionClient<mst_object_localization::SignTrackerAction> m_action_client;

    //! Action client for park sign clustering
    actionlib::SimpleActionClient<mst_object_localization::ParkAction> m_action_client_park;

    //! Sign markers subscriber.
    ros::Subscriber m_sign_markers_subscriber;

    //! Projected point cloud subscriber.
    ros::Subscriber m_projected_cloud_subscriber;

    //! No red light subscriber.
    ros::Subscriber m_no_red_light_subscriber;

    //! Finish line subscriber.
    ros::Subscriber m_finish_line_subscriber;

    ros::Subscriber m_sign_stop_subscriber;

    ros::Subscriber m_autonomous_subscriber;

    //! Park sign marker publisher.
    ros::Publisher m_sign_array_publisher;

    //! The area which park sign will be search publisher.
    ros::Publisher m_area_publisher;

    //! Publisher for the point cloud inside the box.
    ros::Publisher m_point_cloud_publisher;

    //! ActionServer.
    actionlib::SimpleActionServer<mst_object_localization::DeleteSignAction> m_action_server;

    //! Projected cloud topic name parameter.
    std::string m_cloud_topic;

    //! Finish line topic name parameter.
    std::string m_sign_markers_topic;

    //! No red light topic name parameter.
    std::string m_no_red_light_topic;

    //! Finish line topic name.
    std::string m_finish_topic;

    std::string m_sign_stop_topic;

    std::string m_autonomous_topic;

    //! Object count parameter.
    int m_object_count;

    //! Tracker box parameter.
    std::map<std::string, float> m_tracker_box;

    //! The x value that the we'll stop tracking that sign. Parameter.
    int m_sign_drop_x;

    //! The histogram that will store object counter for each sign.
    std::array<int, 24> m_object_histogram;

    // TODO: This vector may be replaced with visualization_msgs::MarkerArray.
    // ROS has data types for our "Sign" class and this vector.
    // We should consider using built-in ROS data types instead of creating custom ones.
    //! The vector which will store signs that we're tracking.
    std::vector<Sign> m_signs;

    int m_park_id;

    int m_no_park_id;

    int m_red_sign_id;

    int m_green_sign_id;

    int m_yellow_sign_id;

    int m_distance;

    //! Incoming point cloud.
    pcl::PointCloud<pcl::PointXYZ>::Ptr m_point_cloud_raw;

    double m_park_distance;

    double m_park_last_x;
    double m_park_last_y;
    double m_park_last_z;

    //! Finish flag.
    bool m_is_finished;

    bool m_first_time;

    std::vector<int> m_ignore_signs;

    // Parametre
    int m_light_count;

    // Sayaç
    int m_light_counter;

    bool m_is_sign_stop;

    int m_station_sign_id;

    bool m_is_autonomous_mode;

};

}  // namespace mst_object_localization
