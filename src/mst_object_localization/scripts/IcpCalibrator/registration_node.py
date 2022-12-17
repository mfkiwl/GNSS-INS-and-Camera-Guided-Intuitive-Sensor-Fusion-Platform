#! /usr/bin/env python3

import rospy
import numpy as np
import open3d as o3d
import ros_numpy
from tf import transformations

from sensor_msgs.msg import PointCloud2, PointField

import global_registration
import icp_registration


class RegistrationNode:
    def __init__(self) -> None:
        self.getParams()

        rospy.Subscriber(self.first_pc2_topic, PointCloud2,
                         self.first_cloud_callback, queue_size=1)

        rospy.Subscriber(self.second_pc2_topic, PointCloud2,
                         self.second_cloud_callback, queue_size=1)

        self.first_pc2 = None
        self.second_pc2 = None

        rate = rospy.Rate(20)
        while not rospy.is_shutdown():
            if self.first_pc2 is not None and self.second_pc2 is not None:
                self.registerGlobal()
                rospy.signal_shutdown("Registered clouds.")
            rospy.loginfo("Waiting point cloud messages...")
            rate.sleep()

    def first_cloud_callback(self, msg: PointCloud2) -> None:
        if self.first_pc2 is None:
            self.first_pc2 = msg

    def second_cloud_callback(self, msg: PointCloud2) -> None:
        if self.second_pc2 is None:
            self.second_pc2 = msg

    def getParams(self):
        self.first_pc2_topic = rospy.get_param("~first_pc2_topic")
        self.second_pc2_topic = rospy.get_param("~second_pc2_topic")
        self.icp_mode = rospy.get_param("~icp_mode")

        rospy.loginfo("First cloud topic: %s", self.first_pc2_topic)
        rospy.loginfo("Second cloud topic: %s", self.second_pc2_topic)
        rospy.loginfo("ICP mode: %s", self.icp_mode)

    def rosToNumpy(self, msg: PointCloud2) -> np.ndarray:
        pc2_numpy = ros_numpy.point_cloud2.pointcloud2_to_array(msg)
        points_list = []
        for i in pc2_numpy:
            item = []
            item.append(i[0])
            item.append(i[1])
            item.append(i[2])
            points_list.append(item)

        return np.asarray(points_list)

    def rosToOpen3D(self, msg: PointCloud2) -> o3d.geometry.PointCloud:
        cloud_numpy = self.rosToNumpy(msg)

        cloud = o3d.geometry.PointCloud()
        cloud.points = o3d.utility.Vector3dVector(cloud_numpy)

        return cloud

    def point2PointICP(self, source, target, trans_init):
        threshold = 0.02
        reg_p2p = icp_registration.registerPoint2PointICP(
            source, target, threshold, trans_init)
        print(f"ICP P2Point Transformation:\n{reg_p2p.transformation}\n\n")

        print("Translation:\n", reg_p2p.transformation[:3, -1])

        rotation_qua = transformations.quaternion_from_matrix(
            reg_p2p.transformation)
        print("Rotation (quaternion):\n", rotation_qua)

        rotation_euler = transformations.euler_from_matrix(
            reg_p2p.transformation)
        print("Rotation (Euler):\n", rotation_euler)

    def point2PlaneICP(self, source, target, trans_init):
        threshold = 0.02
        reg_p2p = icp_registration.registerPoint2PlaneICP(
            source, target, threshold, trans_init)
        print(f"ICP P2Plane Transformation:\n{reg_p2p.transformation}\n\n")

        print("Translation:\n", reg_p2p.transformation[:3, -1])

        rotation_qua = transformations.quaternion_from_matrix(
            reg_p2p.transformation)
        print("Rotation (quaternion):\n", rotation_qua)

        rotation_euler = transformations.euler_from_matrix(
            reg_p2p.transformation)
        print("Rotation (Euler):\n", rotation_euler)

    def registerGlobal(self):
        rospy.loginfo("Received both PointClouds.")

        first_cloud = self.rosToOpen3D(self.first_pc2)
        second_cloud = self.rosToOpen3D(self.second_pc2)

        # First, use global registratoion to calculate initial tranform for ICP.
        global_registration.draw_registration_result(first_cloud, second_cloud, np.asarray([[0.0, 0.0, 1.0, 0.0], [1.0, 0.0, 0.0, 0.0],
                                                                                            [0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 0.0, 1.0]]))

        voxel_size = 0.05
        source, target, source_down, target_down, source_fpfh, target_fpfh = global_registration.prepare_dataset(
            first_cloud, second_cloud, voxel_size)

        result_ransac = global_registration.execute_global_registration(source_down, target_down,
                                                                        source_fpfh, target_fpfh,
                                                                        voxel_size)
        print(result_ransac)
        global_registration.draw_registration_result(source_down, target_down,
                                                     result_ransac.transformation)
        print(f"Global Transformation:\n{result_ransac.transformation}\n\n")

        # ICP mode selection.
        if self.icp_mode == "point":
            self.point2PointICP(first_cloud, second_cloud,
                                result_ransac.transformation)
        elif self.icp_mode == "plane":
            self.point2PlaneICP(first_cloud, second_cloud,
                                result_ransac.transformation)
        else:
            rospy.logerr(
                "Invalid ICP mode. Possible modes are 'point' or 'plane'.")


def main():
    rospy.init_node("lane_3D", anonymous=False)
    node = RegistrationNode()


if __name__ == "__main__":
    main()
