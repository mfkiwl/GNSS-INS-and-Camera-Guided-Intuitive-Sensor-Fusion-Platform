#! /usr/bin/env python3

import torch
from lanenet_model.lanenet.LaneNet import LaneNet
from torchvision import transforms

import numpy as np
import cv2
from PIL import Image as Im

import rospy
from sensor_msgs.msg import Image
from cv_bridge import CvBridge

class Detect():
    def __init__(self) -> None: 
        
        self.model_type = rospy.get_param('~model_type')
        self.model_path = rospy.get_param('~lanenet_weight')
        self.width = rospy.get_param('~width')
        self.height = rospy.get_param('~height')

        self.bridge = CvBridge()

        self.cv_image = None
        self.data_transform = None
        self.device = None
        self.model = None

        self.Device()
        self.Model()
        self.data_tf()

        rospy.Subscriber(rospy.get_param("~image_topic"), Image, self.callback, queue_size=1)

    def Model(self):

        self.model = LaneNet(arch=self.model_type)
        state_dict = torch.load(self.model_path)
        self.model.load_state_dict(state_dict)
        self.model.eval()
        self.model.to(self.device)

        rospy.loginfo('MODEL LOADED.')

    def Device(self):
        
        self.device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")

        rospy.loginfo('DEVİCE LOADED.')

    def data_tf(self):

        self.data_transform = transforms.Compose([
        transforms.Resize((self.height,  self.width)),
        transforms.ToTensor(),
        transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])
        ])

        rospy.loginfo('DATA TRANFORMER LOADED.')

    def callback(self, input_image):
        
        self.cv_image = self.bridge.imgmsg_to_cv2(input_image, desired_encoding="")

    def detect_lane(self):

        if self.cv_image is None:
            return

        color_converter = cv2.cvtColor(self.cv_image, cv2.COLOR_BGR2RGB)
        pil_image = Im.fromarray(color_converter)
        
        image = self.data_transform(pil_image).to(self.device)
        image = torch.unsqueeze(image, dim=0)
        output = self.model(image)
        
        instance_pred = torch.squeeze(output['instance_seg_logits'].detach().to('cpu')).numpy() * 255
        binary_pred = torch.squeeze(output['binary_seg_pred']).to('cpu').numpy() * 255
        instance_pred = instance_pred.transpose(1, 2, 0)

        lanes = np.zeros((binary_pred.shape[0], binary_pred.shape[1], 3), dtype=np.uint8)
        lanes[binary_pred==255] = [0, 255, 0]

        binary_pred_img = self.bridge.cv2_to_imgmsg(lanes, encoding='rgb8')
        print('Detect--------------')

        lane_publisher.lane_pub.publish(binary_pred_img)

class Lane_Publisher():
    def __init__(self) -> None:
        self.lane_pub = rospy.Publisher(
            "/lane_image", Image, queue_size=1
        )

lane_publisher = Lane_Publisher()

def main():

    rospy.init_node("lanenet_publisher_binary_image", anonymous=False)

    detect = Detect()

    rate = rospy.Rate(20)
    while not rospy.is_shutdown():
        detect.detect_lane()
        rate.sleep()

if __name__=='__main__':

    main()