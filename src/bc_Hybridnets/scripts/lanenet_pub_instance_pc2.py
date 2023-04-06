#! /usr/bin/env python3

import torch
from lanenet_model.lanenet.LaneNet import LaneNet
from torchvision import transforms

import numpy as np
import cv2
from PIL import Image as Im

import rospy
from sensor_msgs import point_cloud2
from sensor_msgs.msg import PointCloud2, PointField, Image
from cv_bridge import CvBridge
from std_msgs.msg import Header


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

        self.header = Header()
        self.counter = 0
        self.header.frame_id = None
        self.fields = [PointField('x', 0, PointField.FLOAT32, 1),
                       PointField('y', 4, PointField.FLOAT32, 1),
                       PointField('c', 8, PointField.FLOAT32, 1)]

        rospy.Subscriber(rospy.get_param('~image_topic'), Image, self.callback, queue_size=1)

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
        instance_pred = instance_pred.transpose(1, 2, 0)

        instance_pred = cv2.rotate(instance_pred, cv2.ROTATE_90_COUNTERCLOCKWISE)
        instance_pred = cv2.flip(instance_pred, 0)

        # ------------------------------------------- BGR
        id_list = np.ones(instance_pred.shape[0]*instance_pred.shape[1]).astype('uint8')
        id_counter = 0

        mask_red = np.logical_and(instance_pred[:, :, 0]<10, instance_pred[:, :, 1]<10) # 1
        mask_blue = np.logical_and(instance_pred[:, :, 1]<10, instance_pred[:, :, 2]<10) # 2
        mask_turquoise = np.logical_and(instance_pred[:, :, 0]>250, instance_pred[:, :, 1]>250) # 3
        mask_yellow = np.logical_and(instance_pred[:, :, 1]>250, instance_pred[:, :, 2]>250) # 4
        mask_green = np.logical_and(instance_pred[:, :, 0]<10, instance_pred[:, :, 2]<10) # 5

        points_red = list(zip(list(np.where(mask_red)[0]*4.21875), list(np.where(mask_red)[1]*3.75), id_list+id_counter))
        id_counter += 1
        points_blue = list(zip(list(np.where(mask_blue)[0]*4.21875), list(np.where(mask_blue)[1]*3.75), id_list+id_counter))
        id_counter += 1
        points_turquoise = list(zip(list(np.where(mask_turquoise)[0]*4.21875), list(np.where(mask_turquoise)[1]*3.75), id_list+id_counter))
        id_counter += 1
        points_yellow = list(zip(list(np.where(mask_yellow)[0]*4.21875), list(np.where(mask_yellow)[1]*3.75), id_list+id_counter))
        id_counter += 1
        points_green = list(zip(list(np.where(mask_green)[0]*4.21875), list(np.where(mask_green)[1]*3.75), id_list+id_counter))

        points = points_red + points_blue + points_turquoise + points_yellow + points_green

        self.header.frame_id = str(self.counter)
        self.counter += 1
        self.header.stamp = rospy.Time.now()
        output_cloud = point_cloud2.create_cloud(self.header, self.fields, points)
        # -------------------------------------------

        print('Detected------------------------')
        lane_publisher.lane_pub.publish(output_cloud)

        self.cv_image = None

class Lane_Publisher():
    def __init__(self, pub_topic) -> None:
        self.lane_pub = rospy.Publisher(
            pub_topic, PointCloud2, queue_size=1
        )


def main():

    rospy.init_node("lanenet_publisher_pc2", anonymous=False)

    global lane_publisher
    lane_publisher = Lane_Publisher(rospy.get_param('~pub_topic'))

    detect = Detect()

    rate = rospy.Rate(20)
    while not rospy.is_shutdown():
        detect.detect_lane()
        rate.sleep()

if __name__=='__main__':

    main()