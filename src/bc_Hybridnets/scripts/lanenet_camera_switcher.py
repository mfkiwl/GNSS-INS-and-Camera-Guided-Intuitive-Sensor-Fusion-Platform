#! /usr/bin/env python3

import imp
import torch
from lanenet_model.lanenet.LaneNet import LaneNet
from torchvision import transforms

import numpy as np
import cv2
from PIL import Image as Im

import rospy
from cv_bridge import CvBridge

from sensor_msgs import point_cloud2
from sensor_msgs.msg import PointCloud2, PointField, Image
from std_msgs.msg import Header
from bc_control.msg import controlFlags


class Detect():
    def __init__(self) -> None:

        self.model_type = rospy.get_param('~model_type')
        self.model_path = rospy.get_param('~lanenet_weight')
        self.width = rospy.get_param('~width')
        self.height = rospy.get_param('~height')

        self.bridge = CvBridge()

        self.data_transform = None
        self.device = None
        self.model = None

        self.first_image = None
        self.second_image = None

        self.control_flags = None

        self.current_camera = None

        self.Device()
        self.Model()
        self.data_tf()

        self.header = Header()
        self.counter = 0
        self.header.frame_id = None
        self.fields = [PointField('x', 0, PointField.FLOAT32, 1),
                       PointField('y', 4, PointField.FLOAT32, 1)]

        rospy.Subscriber(rospy.get_param('~first_image_topic'),
                         Image, self.firstImageCallback, queue_size=1)
        rospy.Subscriber(rospy.get_param('~second_image_topic'),
                         Image, self.secondImageCallback, queue_size=1)
        rospy.Subscriber('/control_flags', controlFlags,
                         self.controlFlagsCallback, queue_size=1)

        self.lane_pub = rospy.Publisher(rospy.get_param(
            '~publisher_topic'), PointCloud2, queue_size=1)

    def Model(self):

        self.model = LaneNet(arch=self.model_type)
        state_dict = torch.load(self.model_path)
        self.model.load_state_dict(state_dict)
        self.model.eval()
        self.model.to(self.device)

        rospy.loginfo('MODEL LOADED.')

    def Device(self):

        self.device = torch.device(
            "cuda:0" if torch.cuda.is_available() else "cpu")

        rospy.loginfo('DEVİCE LOADED.')

    def data_tf(self):

        self.data_transform = transforms.Compose([
            transforms.Resize((self.height,  self.width)),
            transforms.ToTensor(),
            transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])
        ])

        rospy.loginfo('DATA TRANFORMER LOADED.')

    def firstImageCallback(self, input_image):

        self.first_image = self.bridge.imgmsg_to_cv2(
            input_image, desired_encoding="")

    def secondImageCallback(self, input_image):

        self.second_image = self.bridge.imgmsg_to_cv2(
            input_image, desired_encoding="")

    def controlFlagsCallback(self, data):

        self.control_flags = data

    def cameraSwitcher(detect_lane):

        def wrapper(self):

            if self.control_flags == None or self.control_flags.path_choice:
                self.current_camera = "first_camera"
            else:
                self.current_camera = "second_camera"
            
            detect_lane(self)
        return wrapper

    @cameraSwitcher
    def detect_lane(self):

        cv_image = None

        if self.current_camera == "first_camera" and self.first_image is not None:
            cv_image = self.first_image
        elif self.current_camera == "second_camera" and self.second_image is not None:
            cv_image = self.second_image
        else:
            rospy.loginfo('Waiting for images.')
            return

        color_converter = cv2.cvtColor(cv_image, cv2.COLOR_BGR2RGB)
        pil_image = Im.fromarray(color_converter)

        image = self.data_transform(pil_image).to(self.device)
        image = torch.unsqueeze(image, dim=0)
        output = self.model(image)

        # instance_pred = torch.squeeze(output['instance_seg_logits'].detach().to('cpu')).numpy() * 255
        binary_pred = torch.squeeze(
            output['binary_seg_pred']).to('cpu').numpy() * 255
        # instance_pred = instance_pred.transpose(1, 2, 0)

        lanes = np.zeros(
            (binary_pred.shape[0], binary_pred.shape[1], 3), dtype=np.uint8)
        lanes[binary_pred == 255] = [0, 255, 0]

        lanes = cv2.resize(
            lanes, (cv_image.shape[1], cv_image.shape[0]))
        lanes = cv2.rotate(lanes, cv2.ROTATE_90_COUNTERCLOCKWISE)
        lanes = cv2.flip(lanes, 0)

        points = zip(list(np.where(lanes == 255)[0]), list(
            np.where(lanes == 255)[1]))

        self.header.frame_id = str(self.counter)
        self.counter += 1
        self.header.stamp = rospy.Time.now()
        output_cloud = point_cloud2.create_cloud(
            self.header, self.fields, list(points))

        print('Detect--------------')

        self.lane_pub.publish(output_cloud)


def main():

    rospy.init_node("lanenet_publisher_pc2", anonymous=False)

    detect = Detect()

    rate = rospy.Rate(20)
    while not rospy.is_shutdown():

        detect.detect_lane()
        rate.sleep()


if __name__ == '__main__':

    main()
