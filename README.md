# GNSS-INS-and-Camera-Guided-Intuitive-Sensor-Fusion-Platform


**The aim of This Project**


The project is an intuitive sensor fusion platform that includes Lidar, IMU, GPS and camera sensors for use in autonomous vehicles. 
Among the goals and motivations;

- Gathering Lidar, GPS, IMU and camera sensors used in different locations in autonomous vehicles on a single platform.
- To propose a solution to the problem of trust, which is seen as a problem in the autonomous sector.
- Since the beginning of the 2000s, it is possible to keep up with the rapidly developing technology age and to contribute to this phenomenon.

This project consists of 4 main modules:

|**Module** |Aim|
|-----|----|
| **Lane Detection** |If the vehicle deviates from the lane by detecting the lanes, it will send a warning to the driver by determining the deviation angle.| 
| **Lidar Camera Calibration** |By performing a calibration between the lidar camera, the depths of the objects that may come in front of the vehicle are determined according to the vehicle, and then the driver is warned using the emergency warning system.|
| **Object Detection** |It provides detection of traffic lights and traffic signs that may come across the vehicle.| 
| **State Estimation** |By calibrating the GPS IMU, the position, speed and angular velocity of the vehicle on the ground are determined.| 

## DEPENDENCIES ##
You have to install PyTorch, PCL and OpenCV libraries. For the rest of the dependencies you can run this command:


`rosdep install --from-paths src --ignore-src -r -y` 

## Running Object Detection Module ##

`roslaunch object_detection yolov7.launch` 


## Running Lane Detection Module ##

`roslaunch bc_Hybridnets hybridnets.launch` 


## Running Lidar Camera Calibration Module ##

`roslaunch livox_ros_driver livox_lidar,rviz.launch` 


## Running State Estimation Module ##

`roslaunch vehicle_state_estimator vehicle_state_estimator.launch` 



