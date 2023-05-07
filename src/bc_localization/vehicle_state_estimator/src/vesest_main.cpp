#include <cstdint>
#include <iostream>
#include <vector>
#include <string>

#include "signal.h"
#include "stdio.h"
#include "ros/duration.h"
#include "ros/ros.h"

#include "msgs_bc/ObstacleKinematics.h"
#include "msgs_bc/imu.h"
#include "msgs_bc/kia_vehicle_states.h"
#include "sensor_msgs/NavSatFix.h"
#include "std_msgs/Float64.h"
#include "std_msgs/Float32.h"
#include "sensor_msgs/Imu.h"
#include "nav_msgs/Odometry.h"

#include "InputAdaptions.h"
#include "VehStEstimn2.h"
#include "tf/tf.h"
#include <ctime>

using namespace std;

VehStEstimnClass::ExtU_VehStEstimn_T VehStEstimn_In;

std_msgs::Header header;

std_msgs::Float64 outputMsg1;

ros::Subscriber VehicleSpeedSub;
ros::Subscriber VehicleGPSSub;
ros::Subscriber VehicleIMUSub;

ros::Publisher pubEKF;
ros::Publisher pubPURE;
ros::Publisher pubPredicted;
double prev_time = 0.0;
/*
void vehiclespeedCallback(const std_msgs::Float32::ConstPtr &submsg)
{
  // double received_secs =ros::Time::now().toSec();
  // std::cout << "Speed received: " << submsg->wheelSpeed[0].data << " \n ";
  // received_secs - prev_time<<std::endl;
  // prev_time = received_secs;
  VehStEstimn_In.CC_VVehFromCan = submsg->data * 3.6;
  // std::cout << "Speed received: " << VehStEstimn_In.CC_VVehFromCan << " " <<
  // received_secs - prev_time<<std::endl;
  // outputMsg1.data = VehStEstimn_In.CC_VVehFromCan;
}*/

void vehiclespeedCallback(const nav_msgs::OdometryConstPtr &submsg)
{
  // VehStEstimn_In.CC_VVehFromCan = submsg->wheelSpeed[0].data;
  VehStEstimn_In.CC_VVehFromCan = submsg->twist.twist.linear.x;
}

/*
void vehicleIMUCallback(const sensor_msgs::Imu::ConstPtr &submsg)
{
  // tepe cek
  VehStEstimn_In.CC_RateYawIMU = submsg->angular_velocity.z;    // check yaw rate is around Z-axis???
  VehStEstimn_In.CC_AVehIMULgt = submsg->linear_acceleration.x; // check longitudinal acc X-axis???
  VehStEstimn_In.CC_AVehIMULat = submsg->linear_acceleration.y; // check Lateral acc Y-axis???
} */

void vehicleIMUCallback(const sensor_msgs::ImuConstPtr &submsg)
{
  // tepe cek
  VehStEstimn_In.CC_RateYawIMU = submsg->angular_velocity.z;    // check yaw rate is around Z-axis???
  VehStEstimn_In.CC_AVehIMULgt = submsg->linear_acceleration.x; // check longitudinal acc X-axis???
  VehStEstimn_In.CC_AVehIMULat = submsg->linear_acceleration.y; // check Lateral acc Y-axis???
}

void vehicleGPSCallback(const sensor_msgs::NavSatFix::ConstPtr &submsg)
{
  VehStEstimn_In.CC_PosnGpsLati = submsg->latitude;
  VehStEstimn_In.CC_PosnGpsLgt = submsg->longitude;
  VehStEstimn_In.CC_PosnGpsAlti = submsg->altitude;
}

template <class T>
bool getParameter(const std::string &param, T &var, ros::NodeHandle &nh_)
{
  if (!nh_.hasParam(param))
  {
    ROS_ERROR_STREAM("[object_filter_node] Parameter '"
                     << param << "' is not defined! Please check the configuration file" << std::flush);

    return false;
  }
  else
  {
    nh_.getParam(param, var);
    return true;
  }
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "vehicle_state_estimator_node");
  ros::NodeHandle n("~");
  std::string vehicle_internal_state_topic;
  std::string gps_topic;
  std::string imu_topic;
  std::string publish_topic;

  getParameter("vehicle_internal_state_topic", vehicle_internal_state_topic, n);
  getParameter("gps_topic", gps_topic, n);
  getParameter("imu_topic", imu_topic, n);
  getParameter("publish_topic", publish_topic, n);

  /* VehicleSpeedSub = n.subscribe<std_msgs::Float32>(
      vehicle_internal_state_topic, 10, vehiclespeedCallback,
      ros::TransportHints().tcpNoDelay(true)); */
  VehicleSpeedSub = n.subscribe<nav_msgs::Odometry>(
      vehicle_internal_state_topic, 1, vehiclespeedCallback,
      ros::TransportHints().tcpNoDelay(true));
  VehicleGPSSub = n.subscribe<sensor_msgs::NavSatFix>(gps_topic, 10, vehicleGPSCallback,
                                                      ros::TransportHints().tcpNoDelay(true));
  VehicleIMUSub = n.subscribe<sensor_msgs::Imu>(imu_topic, 1, vehicleIMUCallback,
                                              ros::TransportHints().tcpNoDelay(true));
  /* VehicleIMUSub = n.subscribe<sensor_msgs::Imu>(imu_topic, 10, vehicleIMUCallback,
                                                ros::TransportHints().tcpNoDelay(true));*/

  pubEKF = n.advertise<msgs_bc::ObstacleKinematics>("EKF_Output", 10);
  pubPredicted = n.advertise<msgs_bc::ObstacleKinematics>("Predicted_Output", 10);
  pubPURE = n.advertise<msgs_bc::ObstacleKinematics>("Pure_Output", 10);

  Transforms transform{};
  double LLA2ECEF1[3], VeSE_Coords[3], IMU_meas_added[4], ENU[1][3];
  VehStEstimnClass VehStEstimn2_In{};
  int VeSE_IdxSnsrUpd[7];
  ros::Rate loop_rate(3);

  Eigen::MatrixXd Predict_state = Eigen::MatrixXd::Random(7, 1);
  Predict_state << 0, 0, 0, 0.5, 0.01, 0.1, 0.1; // VehStEstimn2_In.VeSE_StUpd_Prev;
  Eigen::MatrixXd Predict_covar = Eigen::MatrixXd::Identity(7, 7);
  Predict_covar << 25.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, // Q MATRIX
      0.00, 25.00, 0.00, 0.00, 0.00, 0.00, 0.00,
      0.00, 0.00, 1.00, 0.00, 0.00, 0.00, 0.00,
      0.00, 0.00, 0.00, 0.10, 0.00, 0.00, 0.00,
      0.00, 0.00, 0.00, 0.00, 1.10, 0.00, 0.00,
      0.00, 0.00, 0.00, 0.00, 0.00, 2.00, 0.00,
      0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 2.00; // VehStEstimn2_In.VeSE_ValMtrxUpdCovari_Prev;    // Q

  Eigen::MatrixXd SensorUpd = Eigen::MatrixXd::Random(7, 1);
  Eigen::MatrixXd MeasdState = Eigen::MatrixXd::Random(7, 1);

  Eigen::MatrixXd CovariOut = Eigen::MatrixXd::Identity(7, 7);
  Eigen::MatrixXd StUpdOut = Eigen::MatrixXd::Random(7, 1);
  double East = 0.0, North = 0.0, Up = 0.0, Yaw = 0.0, Vel = 0.0, YawRate = 0.0, Bearing = 0.0, a_lgt = 0.0;

  msgs_bc::ObstacleKinematics KalmanMsg;
  msgs_bc::ObstacleKinematics PureMsg;
  msgs_bc::ObstacleKinematics PredictedStateMsg;
  int loop_idx = 0;
  while (ros::ok())
  {

    transform.LLA2ECEF1(VehStEstimn_In.CC_PosnGpsLati, VehStEstimn_In.CC_PosnGpsLgt, VehStEstimn_In.CC_PosnGpsAlti, LLA2ECEF1);
    transform.LLA2ECEF(VehStEstimn_In.CC_PosnGpsLati, VehStEstimn_In.CC_PosnGpsLgt, VehStEstimn_In.CC_PosnGpsAlti, VeSE_Coords);
    transform.ECEF2ENU(VeSE_Coords[0], VeSE_Coords[1], VeSE_Coords[2], LLA2ECEF1, ENU);
    transform.IMUtransform(VehStEstimn_In.CC_VVehFromCan, VehStEstimn_In.CC_RateYawIMU, VehStEstimn_In.CC_AVehIMULgt, VehStEstimn_In.CC_AVehIMULat, IMU_meas_added);
    Bearing = VehStEstimn2_In.HeadingAngleCalculation(VehStEstimn_In.CC_PosnGpsLgt, VehStEstimn_In.CC_PosnGpsLati);
    VehStEstimn2_In.MeasurementUpdateChecker(ENU[0][0], ENU[0][1], IMU_meas_added[0], IMU_meas_added[1], IMU_meas_added[2], IMU_meas_added[3], Bearing, VeSE_IdxSnsrUpd);

    SensorUpd << VeSE_IdxSnsrUpd[0], VeSE_IdxSnsrUpd[1], VeSE_IdxSnsrUpd[2], VeSE_IdxSnsrUpd[3], VeSE_IdxSnsrUpd[4], VeSE_IdxSnsrUpd[5], VeSE_IdxSnsrUpd[6];
    MeasdState << ENU[0][0], ENU[0][1], Bearing, IMU_meas_added[0], IMU_meas_added[1], IMU_meas_added[2], IMU_meas_added[3];

    VehStEstimnClass::Struct result;
    result = VehStEstimn2_In.State_Update(Predict_state, Predict_covar, SensorUpd, MeasdState);
    CovariOut = result.Covariance;
    StUpdOut = result.State;

    East = VehStEstimn2_In.isNanCheck(StUpdOut.coeff(0, 0));
    North = VehStEstimn2_In.isNanCheck(StUpdOut.coeff(1, 0));
    // Up = ENU[0][2];
    Yaw = VehStEstimn2_In.isNanCheck(StUpdOut.coeff(2, 0)); // radian
    // Yaw = VehStEstimn2_In.HeadingAngleContinuityCheck(Yaw);
    Vel = VehStEstimn2_In.isNanCheck(StUpdOut.coeff(3, 0));
    YawRate = VehStEstimn2_In.isNanCheck(StUpdOut.coeff(4, 0));
    a_lgt = VehStEstimn2_In.isNanCheck(StUpdOut.coeff(5, 0));

    std::cout << "East: " << East << ", North: " << North << ", Yaw: " << Yaw << ", Vel: " << Vel << ", YawRate:" << YawRate << ", a_lgt: " << a_lgt << std::endl;

    KalmanMsg.header.stamp = ros::Time::now();
    KalmanMsg.pose.position.x = East;
    KalmanMsg.pose.position.y = North;
    KalmanMsg.pose.position.z = Up;
    KalmanMsg.pose.orientation.w = Yaw;
    KalmanMsg.twist.angular.z = YawRate * 57.2957795;
    KalmanMsg.twist.linear.x = Vel * 3.6;
    KalmanMsg.twist.angular.x = a_lgt;
    pubEKF.publish(KalmanMsg);

    if (loop_idx > 30)
    {
      PureMsg.header.stamp = ros::Time::now();
      PureMsg.pose.position.x = ENU[0][0];
      PureMsg.pose.position.y = ENU[0][1];
      PureMsg.pose.orientation.w = Bearing;
      // PureMsg.pose.orientation.z = yaw_deg_converted * 57.2957795;
      pubPURE.publish(PureMsg);
    }
    loop_idx++;

    ros::spinOnce();

    loop_rate.sleep();
  }

  return 0;
}
