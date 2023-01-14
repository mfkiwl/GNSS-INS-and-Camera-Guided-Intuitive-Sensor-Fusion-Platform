#include "mst_object_localization/LidarTracker.hpp"

namespace mst_object_localization
{
LidarTracker::LidarTracker(ros::NodeHandle& t_node_handle)
  : m_node_handle(t_node_handle)
  , m_action_client("sign_cluster", true)
  //   , m_action_client("Detection/kdtree_tracker", true)
  , m_action_client_park("park_cluster", true)
  , m_point_cloud_raw(new pcl::PointCloud<pcl::PointXYZ>)
  , m_park_distance{ 100 }
  , m_is_finished{ false }
  , m_first_time{ true }
  , m_is_sign_stop{ false }
  , m_light_counter{ 0 }
  , m_action_server(m_node_handle, "delete_sign",
                    boost::bind(&LidarTracker::actionServerCallback, this, _1), false)
{
    if (!readParameters())
    {
        ROS_ERROR("Could not read parameters.");
        ros::requestShutdown();
    }

    ROS_INFO("Point cloud topic: %s", m_cloud_topic.c_str());
    ROS_INFO("Sign markers topic: %s", m_sign_markers_topic.c_str());
    ROS_INFO("Object count: %d", m_object_count);
    ROS_INFO("Traffic light count: %d", m_light_count);
    for (auto a : m_tracker_box)
    {
        ROS_INFO("%s: %f", a.first.c_str(), a.second);
    }
    ROS_INFO("Ignore sign IDs:");
    for (auto id : m_ignore_signs)
    {
        ROS_INFO("%d", id);
    }

    ROS_INFO("Waiting for action server to start.");
    m_action_client.waitForServer();
    m_action_client_park.waitForServer();
    ROS_INFO("Action server started.");
    ROS_INFO("Finish topic: %s", m_finish_topic.c_str());

    m_action_server.start();

    m_object_histogram.fill(0);

    m_sign_markers_subscriber = m_node_handle.subscribe<visualization_msgs::MarkerArray>(
        m_sign_markers_topic, 1, &LidarTracker::signCallback, this);

    m_projected_cloud_subscriber = m_node_handle.subscribe<sensor_msgs::PointCloud2>(
        m_cloud_topic, 1, &LidarTracker::cloudCallback, this);

    m_no_red_light_subscriber =
        m_node_handle.subscribe(m_no_red_light_topic, 10, &LidarTracker::noRedLightCallback, this);

    m_finish_line_subscriber = m_node_handle.subscribe<std_msgs::Bool>(
        m_finish_topic, 1, &LidarTracker::finishCallback, this);

    m_sign_stop_subscriber = m_node_handle.subscribe<std_msgs::Bool>(
        m_sign_stop_topic, 1, &LidarTracker::signStopCallback, this);

    m_autonomous_subscriber = m_node_handle.subscribe<std_msgs::Bool>(
        m_autonomous_topic, 1, &LidarTracker::autonomousCallback, this);

    m_point_cloud_publisher = m_node_handle.advertise<sensor_msgs::PointCloud2>("my_box", 1);

    m_sign_array_publisher =
        m_node_handle.advertise<visualization_msgs::MarkerArray>("tracked_markers", 1);

    m_area_publisher = m_node_handle.advertise<visualization_msgs::MarkerArray>("tracked_area", 1);

    ROS_INFO("Successfully launched node.");
}

bool LidarTracker::readParameters()
{
    if (!m_node_handle.getParam("cloud_topic", m_cloud_topic))
    {
        return false;
    }
    if (!m_node_handle.getParam("object_count", m_object_count))
    {
        return false;
    }
    if (!m_node_handle.getParam("tracker_box", m_tracker_box))
    {
        return false;
    }
    if (!m_node_handle.getParam("sign_markers_topic", m_sign_markers_topic))
    {
        return false;
    }
    if (!m_node_handle.getParam("sign_drop_x", m_sign_drop_x))
    {
        return false;
    }
    if (!m_node_handle.getParam("red_sign_id", m_red_sign_id))
    {
        return false;
    }
    if (!m_node_handle.getParam("green_sign_id", m_green_sign_id))
    {
        return false;
    }
    if (!m_node_handle.getParam("yellow_sign_id", m_yellow_sign_id))
    {
        return false;
    }
    if (!m_node_handle.getParam("distance", m_distance))
    {
        return false;
    }
    if (!m_node_handle.getParam("park_id", m_park_id))
    {
        return false;
    }
    if (!m_node_handle.getParam("no_park_id", m_no_park_id))
    {
        return false;
    }
    if (!m_node_handle.getParam("no_red_light_topic", m_no_red_light_topic))
    {
        return false;
    }
    if (!m_node_handle.getParam("finish", m_finish_topic))
    {
        return false;
    }
    if (!m_node_handle.getParam("ignore_signs", m_ignore_signs))
    {
        return false;
    }
    if (!m_node_handle.getParam("light_count", m_light_count))
    {
        return false;
    }
    if (!m_node_handle.getParam("sign_stop_topic", m_sign_stop_topic))
    {
        return false;
    }
    if (!m_node_handle.getParam("station_sign_id", m_station_sign_id))
    {
        return false;
    }
    if (!m_node_handle.getParam("autonomous_topic", m_autonomous_topic))
    {
        return false;
    }
    return true;
}

void LidarTracker::signStopCallback(const std_msgs::BoolConstPtr& t_is_stopped)
{
    m_is_sign_stop = t_is_stopped->data;
}

void LidarTracker::autonomousCallback(const std_msgs::BoolConstPtr& t_is_autonomous)
{
    m_is_autonomous_mode = t_is_autonomous->data;
    // if (m_is_autonomous_mode)
    // {
    //     std::cout << "Autonomous mode true\n";
    // }
    // else
    // {
    //     std::cout << "Autonomous mode false\n";
    // }
}

void LidarTracker::printActiveSigns()
{
    std::cout << "-----------\n";
    std::cout << "Active Signs Size: " << m_signs.size() << '\n';
    for (auto sign : m_signs)
    {
        std::cout << "---\n";
        std::cout << "ID: " << sign.Class << '\n';
        std::cout << "X : " << sign.x << '\n';
        std::cout << "Y : " << sign.y << '\n';
        std::cout << "Z : " << sign.z << '\n';
    }
}

void LidarTracker::finishCallback(const std_msgs::BoolConstPtr& t_is_finished)
{
    m_is_finished = t_is_finished->data;
    // if (m_is_finished)
    // {
    //     std::cout << "Finish true\n";
    // }
    // else
    // {
    //     std::cout << "Finish false\n";
    // }
}

double LidarTracker::calculateDistance(const double& t_x_coordinate, const double& t_y_coordinate,
                                       const double& t_z_coordinate)
{
    return sqrt(pow(t_x_coordinate, 2.0) + pow(t_y_coordinate, 2.0) + pow(t_z_coordinate, 2.0));
}

void LidarTracker::trackSign(const double& t_x_coordinate, const double& t_y_coordinate,
                             const double& t_z_coordinate)
{
    // Crop the point cloud so that only the sign remains.
    pcl::CropBox<pcl::PointXYZ> cropBoxFilter(true);
    cropBoxFilter.setInputCloud(m_point_cloud_raw);
    Eigen::Vector4f min_pt(t_x_coordinate - m_tracker_box["x_min"],
                           t_y_coordinate - m_tracker_box["y_min"],
                           t_z_coordinate - m_tracker_box["z_min"], 1.0f);
    Eigen::Vector4f max_pt(t_x_coordinate + m_tracker_box["x_max"],
                           t_y_coordinate + m_tracker_box["y_max"],
                           t_z_coordinate + m_tracker_box["z_max"], 1.0f);

    cropBoxFilter.setMin(min_pt);
    cropBoxFilter.setMax(max_pt);

    std::vector<int> indices;
    cropBoxFilter.filter(indices);

    pcl::PointCloud<pcl::PointXYZ> point_cloud_sign;
    cropBoxFilter.filter(point_cloud_sign);

    // Convert PCL cloud to PointCloud2 in order to send goal.
    sensor_msgs::PointCloud2 goal_cloud;
    pcl::toROSMsg(point_cloud_sign, goal_cloud);

    // Create and send goal.
    mst_object_localization::ParkGoal action_goal;
    action_goal.cloud = goal_cloud;
    m_action_client_park.sendGoal(action_goal);

    // Wait for the action to return.
    bool finished_before_timeout = m_action_client_park.waitForResult(ros::Duration(0.2));

    if (finished_before_timeout)
    {
        actionlib::SimpleClientGoalState state = m_action_client_park.getState();
        // ROS_INFO("Action finished: %s", state.toString().c_str());
    }
    else
    {
        ROS_INFO("Action did not finish before the time out.");
    }

    // std::cout << "*******\nDBSCAN GELEN:\n";
    // std::cout << "gelen x: " << m_action_client_park.getResult()->point.x;
    // std::cout << "gelen y: " << m_action_client_park.getResult()->point.y;
    // std::cout << "gelen z: " << m_action_client_park.getResult()->point.z;
}

void LidarTracker::actionServerCallback(
    const mst_object_localization::DeleteSignGoalConstPtr& goal)
{
    ROS_INFO("Goal arrived.");

    for (size_t i = 0; i < goal->signs.size(); ++i)
    {
        int index = -1;
        // for (auto active_sign : m_signs)
        // {
        //     if (goal->signs[i] == active_sign.Class)
        //     {
        //         index = i;
        //     }
        // }
        if (index != -1)
        {
            m_signs.erase(m_signs.begin() + index);
            ROS_INFO("* Deleted %d", goal->signs[i]);
        }
    }

    mst_object_localization::DeleteSignResult result;
    result.status = true;
    m_action_server.setSucceeded(result);
}

void LidarTracker::signCallback(const visualization_msgs::MarkerArrayConstPtr& t_marker_array)
{
    // ******** EN YAKIN PARK ********
    // std::cout << "----\n";
    // if (t_marker_array->markers.size() == 0)
    // {
    //     // Sometimes marker array arrives empty. In that case we shouldn't continue rest.
    //     return;
    // }

    // if (m_is_finished)
    // {
    //     // Pair: id, distance
    //     std::vector<std::pair<int, double>> sign_distances;

    //     int counter = 0;

    //     bool is_park = false;

    //     for (auto marker : t_marker_array->markers)
    //     {
    //         if (t_marker_array->markers[counter].id == m_park_id)
    //         {
    //             double sign_distance =
    //                 calculateDistance(t_marker_array->markers[counter].pose.position.x,
    //                                   t_marker_array->markers[counter].pose.position.y, 0);
    //             sign_distances.push_back(std::pair<int, double>{ counter, sign_distance });
    //             // std::cout << "dist: " << sign_distance << '\n';
    //         }
    //         ++counter;
    //     }

    //     // We may see other signs rather than park.
    //     // In that case, "sign_distances" vector may contain other signs and it may be empty.
    //     // Because we push_back only park signs.
    //     if (sign_distances.size() != 0)
    //     {
    //         is_park = true;
    //     }

    //     if (is_park)
    //     {
    //         //  Compare only second elements of pairs and get the miminum value of the vector,
    //         // which
    //         //  is the nearest park sign.
    //         std::pair<int, double> nearest_sign = *std::min_element(
    //             sign_distances.cbegin(), sign_distances.cend(),
    //             [](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second; });

    //         // std::cout << "nearest: " << nearest_sign.second << '\n';

    //         // std::cout << "yeşil x: " <<
    //         // t_marker_array->markers[nearest_sign.first].pose.position.x
    //         //           << '\n';
    //         // std::cout << "yeşil y: " <<
    //         // t_marker_array->markers[nearest_sign.first].pose.position.y
    //         //           << '\n';
    //         // std::cout << "yeşil z: " <<
    //         // t_marker_array->markers[nearest_sign.first].pose.position.z
    //         //           << '\n';

    //         trackSign(t_marker_array->markers[nearest_sign.first].pose.position.x,
    //                   t_marker_array->markers[nearest_sign.first].pose.position.y,
    //                   t_marker_array->markers[nearest_sign.first].pose.position.z);

    //         double box_x_min = m_park_last_x - m_tracker_box["x_min"];
    //         double box_x_max = m_park_last_x + m_tracker_box["x_max"];
    //         double box_y_min = m_park_last_y - m_tracker_box["y_min"];
    //         double box_y_max = m_park_last_y + m_tracker_box["y_max"];
    //         double box_z_min = m_park_last_z - m_tracker_box["z_min"];
    //         double box_z_max = m_park_last_z + m_tracker_box["z_max"];

    //         double new_distance = calculateDistance(m_action_client_park.getResult()->point.x,
    //                                                 m_action_client_park.getResult()->point.y,
    //                                                 0);

    //         if (!((box_x_min < m_action_client_park.getResult()->point.x) &&
    //               (m_action_client_park.getResult()->point.x < box_x_max) &&
    //               (box_y_min < m_action_client_park.getResult()->point.y) &&
    //               (m_action_client_park.getResult()->point.y < box_y_max) &&
    //               (box_z_min < m_action_client_park.getResult()->point.z) &&
    //               (m_action_client_park.getResult()->point.z < box_z_max)) &&
    //             !m_first_time)
    //         {
    //             std::cout << "Kutunun dışında\n";
    //             if (m_point_cloud_raw->size() != 0 && new_distance < m_park_distance)
    //             {
    //                 std::cout << "Distance küçük\n";
    //                 m_park_distance = new_distance;

    //                 for (auto& sign : m_signs)
    //                 {
    //                     if (sign.id == m_park_id)
    //                     {
    //                         sign.x = m_action_client_park.getResult()->point.x;
    //                         sign.y = m_action_client_park.getResult()->point.y;
    //                         sign.z = m_action_client_park.getResult()->point.z;
    //                     }
    //                 }

    //                 // FOR İLE m_signs' I DÖN VE ESKİ MARKERI GÜNCELLE !!!!!
    //             }
    //         }
    //         else if (m_first_time)
    //         {
    //             m_first_time = false;

    //             if (m_point_cloud_raw->size() != 0 && new_distance < m_park_distance)
    //             {
    //                 std::cout << "Tabela ilk defa geldi.\n";
    //                 m_park_distance = new_distance;

    //                 Sign my_sign;
    //                 my_sign.id = m_park_id;
    //                 my_sign.x = m_action_client_park.getResult()->point.x;
    //                 my_sign.y = m_action_client_park.getResult()->point.y;
    //                 my_sign.z = m_action_client_park.getResult()->point.z;
    //                 m_signs.push_back(my_sign);
    //                 m_object_histogram[m_park_id] = 0;
    //             }
    //         }

    //         // std::cout << "-------\ntracked x: " << m_action_client_park.getResult()->point.x
    //         //           << '\n';
    //         // std::cout << "tracked y: " << m_action_client_park.getResult()->point.y << '\n';
    //         // std::cout << "tracked z: " << m_action_client_park.getResult()->point.z << '\n';
    //     }
    // }

    for (auto marker : t_marker_array->markers)
    {
        bool is_ignore = false;
        for (auto id : m_ignore_signs)
        {
            if (marker.id == id)
            {
                ROS_INFO("Ignoring sign %d", id);
                is_ignore = true;
            }
        }
        if (is_ignore)
        {
            continue;
        }

        // // ******** EN YAKIN PARK ********
        // if (marker.id == m_park_id)
        // {
        //     continue;
        // }

        if (!m_is_finished)
        {
            if (marker.id == m_park_id)
            {
                std::cout << "Received park in parkour.\n ";
                continue;
            }
        }

        if (m_is_finished && (marker.id != m_no_park_id && marker.id != m_park_id))
        {
            ROS_INFO("Finish mode is active but [ %d ] received.", marker.id);
            continue;
        }

        if (marker.id != m_park_id && marker.id != m_no_park_id)
        {
            // std::cout << "id: " << marker.id << '\n';
            // std::cout << "x: " << marker.pose.position.x << '\n';
            if (marker.pose.position.x > m_distance)
            {
                continue;
            }
        }
        // Check m_signs vector to see if the sign already exists.
        bool is_exist{ false };
        // for (auto sign : m_signs)
        for (size_t i = 0; i < m_signs.size(); i++)
        {
            if (m_signs[i].id == marker.id)
            {
                is_exist = true;
                continue;
            }
        }

        // If the vector doesn't have the sign, increase its counter.
        if (!is_exist)
        {
            if (marker.id == m_red_sign_id || marker.id == m_yellow_sign_id ||
                marker.id == m_green_sign_id)
            {
                ++m_light_counter;
                ROS_INFO("Traffic light counter: %d", m_light_counter);
            }
            else
            {
                if (marker.id == m_station_sign_id && m_is_sign_stop)
                {
                    m_object_histogram[marker.id] = 0;
                }
                else
                {
                    if (!m_is_autonomous_mode && marker.id == m_station_sign_id)
                    {
                        std::cout << "############################# OTONOM MODDA DEĞİLKEN DURAK GELDİ.\n";
                        continue;
                    }
                    ++m_object_histogram[marker.id];
                    ROS_INFO("Object %d counter: %d", marker.id, m_object_histogram[marker.id]);
                }
            }
        }

        bool normal_sign = m_object_histogram[marker.id] > m_object_count;
        bool traffic_light = (m_light_counter > m_light_count &&
                              (marker.id == m_red_sign_id || marker.id == m_yellow_sign_id ||
                               marker.id == m_green_sign_id));

        // If the counter is satisfied, add the sign to the vector and reset the counter.
        if (!is_exist && (normal_sign || traffic_light))
        {
            // If the marker is green sign, then delete red and yellow sign markers.
            if (marker.id == m_green_sign_id)
            {
                // Check m_signs vector to see if the sign already exists.
                int index_red = -1;
                int index_yellow = -1;
                // for (auto sign : m_signs)
                for (size_t i = 0; i < m_signs.size(); i++)
                {
                    if (m_signs[i].id == m_red_sign_id)
                    {
                        index_red = i;
                        continue;
                    }
                }
                if (index_red != -1)
                {
                    m_signs.erase(m_signs.begin() + index_red);
                    std::cout << "Kırmızı silndi\n";
                }

                for (size_t i = 0; i < m_signs.size(); i++)
                {
                    if (m_signs[i].id == m_yellow_sign_id)
                    {
                        index_yellow = i;
                        continue;
                    }
                }

                if (index_yellow != -1)
                {
                    m_signs.erase(m_signs.begin() + index_yellow);
                }
            }

            // If the marker is red or yellow sign, then delete green sign markers.
            if (marker.id == m_red_sign_id || marker.id == m_yellow_sign_id)
            {
                // Check m_signs vector to see if the sign already exists.
                bool is_exist{ false };
                int index;
                // for (auto sign : m_signs)
                for (size_t i = 0; i < m_signs.size(); i++)
                {
                    if (m_signs[i].id == m_green_sign_id)
                    {
                        is_exist = true;
                        index = i;
                        continue;
                    }
                }
                if (is_exist)
                {
                    m_signs.erase(m_signs.begin() + index);
                }
            }

            Sign my_sign;
            my_sign.id = marker.id;
            my_sign.x = marker.pose.position.x;
            my_sign.y = marker.pose.position.y;
            my_sign.z = marker.pose.position.z;
            m_signs.push_back(my_sign);
            if (marker.id == m_red_sign_id || marker.id == m_yellow_sign_id ||
                marker.id == m_green_sign_id)
            {
                m_light_counter = 0;
            }
            else
            {
                m_object_histogram[marker.id] = 0;
            }
        }
    }
}

void LidarTracker::noRedLightCallback(const std_msgs::BoolConstPtr& t_is_delete)
{
    std::cout << "**** CALLBACK BAŞLADI ****\n";
    std::cout << t_is_delete->data << '\n';
    if (t_is_delete->data)
    {
        // Delete red and yellow light markers.

        // Check m_signs vector to see if the sign already exists.
        int index_red = -1;
        for (size_t i = 0; i < m_signs.size(); i++)
        {
            if (m_signs[i].Class== "red")
            {
                index_red = i;
                continue;
            }
        }
        if (index_red != -1)
        {
            m_signs.erase(m_signs.begin() + index_red);
            std::cout << "Kırmızı silndi\n";
        }

        // Check m_signs vector to see if the sign already exists.
        int index_yellow = -1;
        for (size_t i = 0; i < m_signs.size(); i++)
        {
            if (m_signs[i].id == m_yellow_sign_id)
            {
                index_yellow = i;
                continue;
            }
        }
        if (index_yellow != -1)
        {
            m_signs.erase(m_signs.begin() + index_yellow);
            std::cout << "Sarı silndi\n";
        }
    }
}

void LidarTracker::cloudCallback(const sensor_msgs::PointCloud2ConstPtr& t_point_cloud)
{
    printActiveSigns();

    // Convert ROS PointCloud2 message to PCL type.
    // pcl::PointCloud<pcl::PointXYZ>::Ptr point_cloud_raw(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromROSMsg(*t_point_cloud, *m_point_cloud_raw);

    // Create goal.
    mst_object_localization::SignTrackerGoal action_goal;

    for (auto sign : m_signs)
    {
        // Crop the point cloud so that only the sign remains.
        pcl::CropBox<pcl::PointXYZ> cropBoxFilter(true);
        cropBoxFilter.setInputCloud(m_point_cloud_raw);
        Eigen::Vector4f min_pt(sign.x - m_tracker_box["x_min"], sign.y - m_tracker_box["y_min"],
                               sign.z - m_tracker_box["z_min"], 1.0f);
        Eigen::Vector4f max_pt(sign.x + m_tracker_box["x_max"], sign.y + m_tracker_box["y_max"],
                               sign.z + m_tracker_box["z_max"], 1.0f);

        cropBoxFilter.setMin(min_pt);
        cropBoxFilter.setMax(max_pt);

        // Bu gerekli mi?
        std::vector<int> indices;
        cropBoxFilter.filter(indices);

        pcl::PointCloud<pcl::PointXYZ> point_cloud_sign;
        cropBoxFilter.filter(point_cloud_sign);

        // Convert PCL cloud to PointCloud2 in order to send goal.
        sensor_msgs::PointCloud2 goal_cloud;
        pcl::toROSMsg(point_cloud_sign, goal_cloud);

        mst_object_localization::PointCloud2WithId point_cloud_sign_with_id;
        point_cloud_sign_with_id.header.stamp = ros::Time::now();
        point_cloud_sign_with_id.Class = sign.Class;
        point_cloud_sign_with_id.cloud = goal_cloud;
        action_goal.clouds.push_back(point_cloud_sign_with_id);

        // Publish croped cloud.
        goal_cloud.header.frame_id = "velodyne";
        goal_cloud.header.stamp = ros::Time::now();
        m_point_cloud_publisher.publish(goal_cloud);
    }

    m_action_client.sendGoal(action_goal);

    // Wait for the action to return.
    bool finished_before_timeout = m_action_client.waitForResult(ros::Duration(0.2));

    if (finished_before_timeout)
    {
        actionlib::SimpleClientGoalState state = m_action_client.getState();
        // ROS_INFO("Action finished: %s", state.toString().c_str());
    }
    else
    {
        ROS_INFO("Action did not finish before the time out.");
    }

    // std::cout << "------------\n";
    // std::cout << "Received action result:\n";
    // std::cout << "MarkerArray Size: " << m_action_client.getResult()->markers.markers.size()
    //           << '\n';
    // for (auto marker : m_action_client.getResult()->markers.markers)
    // {
    //     std::cout << "---\n";
    //     std::cout << "ID: " << marker.id << '\n';
    //     std::cout << "X : " << marker.pose.position.x << '\n';
    //     std::cout << "Y : " << marker.pose.position.y << '\n';
    //     std::cout << "Z : " << marker.pose.position.z << '\n';
    // }

    // We'll stop tracking the sign when the x coordinate becomes less than "m_sign_drop_x" param.
    // This variable will hold the index of the sign when it's time to stop tracking it.
    int sign_drop_index{ -1 };

    for (size_t i = 0; i < m_signs.size(); ++i)
    {
        // Counter for while loop.
        size_t j{ 0 };

        // This bool is for breaking the while loop instead of breaking for loop.
        bool is_updated{ false };

        // We changed this loop to while to get rid of "break" in for loop with my bro Urcu.
        while (!is_updated && j < m_action_client.getResult()->markers.markers.size())
        {
            if (m_signs[i].Class == m_action_client.getResult()->markers.markers[j].Class)
            {
                // We're updating sign coordintaes with the new DBSCAN clustered values.
                m_signs[i].x = m_action_client.getResult()->markers.markers[j].pose.position.x;
                m_signs[i].y = m_action_client.getResult()->markers.markers[j].pose.position.y;
                m_signs[i].z = m_action_client.getResult()->markers.markers[j].pose.position.z;
                is_updated = true;
            }
            ++j;
        }

        if (m_signs[i].x < m_sign_drop_x && m_signs[i].Class!= m_park_id &&
            m_signs[i].Class != m_no_park_id)
        {
            sign_drop_index = i;
        }
    }

    if (sign_drop_index != -1)
    {
        m_signs.erase(m_signs.begin() + sign_drop_index);
    }

    visualization_msgs::MarkerArray sign_array;
    visualization_msgs::MarkerArray sign_area_array;

    for (auto sign : m_signs)
    {
        if (sign.Class == m_park_id)
        {
            m_park_distance = calculateDistance(sign.x, sign.y, sign.z);
            m_park_last_x = sign.x;
            m_park_last_y = sign.y;
            m_park_last_z = sign.z;
        }
        // Publish park sign marker.
        visualization_msgs::Marker sign_marker;
        sign_marker.header.frame_id = "velodyne";
        sign_marker.header.stamp = ros::Time::now();
        sign_marker.id = 1;
        sign_marker.type = sign_marker.SPHERE;
        sign_marker.action = sign_marker.ADD;
        sign_marker.pose.position.x = sign.x;
        sign_marker.pose.position.y = sign.y;
        sign_marker.pose.position.z = sign.z;
        sign_marker.pose.orientation.x = 0;
        sign_marker.pose.orientation.y = 0;
        sign_marker.pose.orientation.z = 0;
        sign_marker.pose.orientation.w = 1;
        sign_marker.scale.x = 0.1;
        sign_marker.scale.y = 0.8;
        sign_marker.scale.z = 0.8;
        sign_marker.color.a = 1;
        sign_marker.color.r = 1;
        sign_marker.color.g = 1;
        sign_marker.color.b = 1;
        sign_marker.lifetime = ros::Duration(0.1);

        sign_array.markers.push_back(sign_marker);

        // Publish park area marker.
        double x_min = sign.x - m_tracker_box["x_min"];
        double y_min = sign.y - m_tracker_box["y_min"];
        double z_min = sign.z - m_tracker_box["z_min"];
        double x_max = sign.x + m_tracker_box["x_max"];
        double y_max = sign.y + m_tracker_box["y_max"];
        double z_max = sign.z + m_tracker_box["z_max"];

        visualization_msgs::Marker area_marker;
        area_marker.header.frame_id = "velodyne";
        area_marker.header.stamp = ros::Time::now();
        area_marker.id = 1;
        area_marker.type = area_marker.CUBE;
        area_marker.action = area_marker.ADD;
        area_marker.pose.position.x = (x_min + x_max) / 2;
        area_marker.pose.position.y = (y_min + y_max) / 2;
        area_marker.pose.position.z = (z_min + z_max) / 2;
        area_marker.pose.orientation.x = 0;
        area_marker.pose.orientation.y = 0;
        area_marker.pose.orientation.z = 0;
        area_marker.pose.orientation.w = 1;
        area_marker.scale.x = x_max - x_min;
        area_marker.scale.y = y_max - y_min;
        area_marker.scale.z = z_max - z_min;
        area_marker.color.a = 0.3;
        area_marker.color.r = 1;
        area_marker.color.g = 0;
        area_marker.color.b = 0;
        area_marker.lifetime = ros::Duration(0.1);

        sign_area_array.markers.push_back(area_marker);
    }

    m_sign_array_publisher.publish(sign_array);
    m_area_publisher.publish(sign_area_array);
}

}  // namespace mst_object_localization