#! /usr/bin/env python3
import numpy as np
import cv2
import rospy
import torch
from torchvision import transforms
from sensor_msgs.msg import Image
from cv_bridge import CvBridge, CvBridgeError
import torch
from hybridnets_model.backbone import HybridNetsBackbone
from torchvision import transforms
from hybridnets_model.utils.constants import *
from PIL import Image as Im
from std_msgs.msg import Header
from sensor_msgs.msg import PointField, PointCloud2
from sensor_msgs import point_cloud2

# from message_filters import ApproximateTimeSynchronizer, Subscriber



# use_cuda = torch.cuda.is_available()

seg_list= ["drivable","lane"]


class hybridnets_ros():
    def __init__(self):

        self.width = rospy.get_param('~width')
        self.height = rospy.get_param('~height')
        
        self.bridge = CvBridge()

        self.weight = rospy.get_param('~weight')
        self.state_dict = torch.load(self.weight)
        self.cv_image = None
        self.data_transform = None
        self.device = None
        self.model = None

        self.header = Header()
        self.header.frame_id = "cam01"
        self.fields = [PointField('x', 0, PointField.FLOAT32, 1),
                       PointField('y', 4, PointField.FLOAT32, 1)]

        self.Device()
        self.Model()
        self.data_tf()
        
        rospy.Subscriber(rospy.get_param("~image_topic"), Image, self.callback, queue_size=1)
        rospy.loginfo('INITIALIZED.')

    def Device(self):
        self.device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
        rospy.loginfo('DEVICE LOADED.')

    def seg_mode(self):
        weight_last_layer_seg = self.state_dict.get('model', self.state_dict)['segmentation_head.0.weight']
        if weight_last_layer_seg.size(0) == 1:
            seg_mode = BINARY_MODE
        else:
            seg_mode = MULTICLASS_MODE
        return seg_mode

    def Model(self):
        self.model = HybridNetsBackbone(compound_coef=3, num_classes=1, seg_classes=len(seg_list), backbone_name=None, seg_mode=self.seg_mode())
        self.model.load_state_dict(self.state_dict, strict=False)
        self.model.requires_grad_(False)
        self.model.eval()
        self.model.to(self.device)
        # if use_cuda:
        #     self.model = self.model.half()
        rospy.loginfo('MODEL LOADED.')

    def callback(self,data):
        try:
            self.cv_image = self.bridge.imgmsg_to_cv2(data, "bgr8")
        except CvBridgeError as e:
            print(e)

    def data_tf(self):

        self.data_transform = transforms.Compose([
        transforms.Resize((self.height,  self.width)),
        transforms.ToTensor(),
        transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])
        ])

        rospy.loginfo('DATA TRANFORMER LOADED.')

    def detect(self):
        seg_mask_list = []
        if self.cv_image is None:
            print("No image")
            return
        image = Im.fromarray(self.cv_image)
        image = self.data_transform(image)
        image = image.unsqueeze(0)
        image = image.to(self.device)
        # if use_cuda:
        #     image = image.half()
        _,_,_,_,seg= self.model(image)

        if self.seg_mode() == BINARY_MODE:
            seg_mask = torch.where(seg >= 0.5, 1, 0)
            seg_mask.squeeze_(1)
            seg_mask_list.append(seg_mask)
        elif self.seg_mode() == MULTICLASS_MODE:
            _, seg_mask = torch.max(seg, 1)
            seg_mask_list.append(seg_mask)
            print(len(seg_mask_list))
        else:
            seg_mask_list = [torch.where(seg[:, i, ...] >= 0.5, 1, 0) for i in range(seg.size(1))]
            # but remove background class from the list
            seg_mask_list.pop(0)

        for i in range(len(seg_mask_list)):
            for _, seg_mask in enumerate(seg_mask_list):

                seg_mask_ = seg_mask[i].squeeze().cpu().numpy()

                lanes = np.zeros((self.height, self.width, 3), dtype=np.uint8)
                lanes[seg_mask_==2] = [0, 255, 0]

                lanes = cv2.resize(lanes, (self.cv_image.shape[1],self.cv_image.shape[0]))
                lanes = cv2.rotate(lanes, cv2.ROTATE_90_COUNTERCLOCKWISE)
                lanes = cv2.flip(lanes, 0)

                points = zip(list(np.where(lanes==255)[0]), list(np.where(lanes==255)[1]))


                self.header.stamp = rospy.Time.now()
                
                output_cloud = point_cloud2.create_cloud(self.header, self.fields, list(points))
                pub_lanes_pc2.lane_pub.publish(output_cloud)


class Lane_Publisher():
    def __init__(self, pub_topic) -> None:
        self.lane_pub = rospy.Publisher(
            pub_topic, PointCloud2, queue_size=1
        )


if __name__ == '__main__':
    rospy.init_node('hybridnets_ros', anonymous=True)
    global pub_lanes_pc2
    pub_lanes_pc2 = Lane_Publisher(rospy.get_param("~pub_topic"))
    detect_object = hybridnets_ros()
    rate = rospy.Rate(10)
    while not rospy.is_shutdown():
        seg_mask_=detect_object.detect()
        rate.sleep()