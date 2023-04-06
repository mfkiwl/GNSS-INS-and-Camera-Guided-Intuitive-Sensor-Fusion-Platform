import cv2
import numpy as np
import rospy
import torch
from torchvision import transforms
from sensor_msgs.msg import Image
from cv_bridge import CvBridge, CvBridgeError
from torch.backends import cudnn
import torch
from torch.backends import cudnn
from backbone import HybridNetsBackbone
from torchvision import transforms
from utils.constants import *
from PIL import Image as Im
from std_msgs.msg import Header
from message_filters import ApproximateTimeSynchronizer, Subscriber


use_cuda = torch.cuda.is_available()


seg_list= ["drivable","lane"]
color_list_seg = {}

class hybridnets_ros():
    def __init__(self):
        # self.weight = '/home/muco/HybridNets/weights/hybridnets.pth'
        # self.weight = rospy.get_param('~weight')
        self.weight = '/home/muco/Downloads/hybridnets-d3_261_11500.pth' 
        self.state_dict = torch.load(self.weight)
        # self.width = rospy.get_param('~width')
        # self.height = rospy.get_param('~height')

        self.width = 640
        self.height = 364
        self.bridge = CvBridge()


        self.cv_image = None
        self.data_transform = None
        self.device = None
        self.model = None

        self.header = Header()
        self.header.frame_id = "cam01"
        self.header.stamp = rospy.Time.now()

        self.Device()
        self.Model()
        self.data_tf()
        

        # rospy.Subscriber(rospy.get_param("~image_topic"), Image, self.callback, queue_size=1)
        rospy.Subscriber("/camera/front", Image, self.callback, queue_size=1)

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
        else:
            seg_mask_list = [torch.where(seg[:, i, ...] >= 0.5, 1, 0) for i in range(seg.size(1))]
            seg_mask_list.pop(0)
        for i in range(len(seg_mask_list)):
            for _, seg_mask in enumerate(seg_mask_list):

                seg_mask_ = seg_mask[i].squeeze().cpu().numpy()
        return seg_mask_

    def lane_publisher(self,seg_mask_):
        lanes = np.zeros((self.height, self.width, 3), dtype=np.uint8)
        lanes[seg_mask_==2] = [0, 255, 0]
        pub_lanes.lane_pub.publish(self.bridge.cv2_to_imgmsg(lanes, "bgr8"))
    
    def drivable_publisher(self,seg_mask_):
        drivable = np.zeros((self.height, self.width, 3), dtype=np.uint8)
        drivable[seg_mask_==1] = [255, 0, 0]
        print(type(drivable))
        pub_drivable.drivable_pub.publish(self.bridge.cv2_to_imgmsg(drivable, "bgr8"))

    def lane_on_original(self,seg_mask_):
        lanes_on_original = self.cv_image
        lanes_on_original=cv2.resize(lanes_on_original,(self.width,self.height))
        lanes_on_original[seg_mask_==2] = [0, 255, 0]
        cv2.imshow("lane", lanes_on_original)

    def drivable_on_original(self,seg_mask_):
        drivable_on_original = self.cv_image
        drivable_on_original=cv2.resize(drivable_on_original,(self.width,self.height))
        drivable_on_original[seg_mask_==1] = [0, 0, 255]
        cv2.imshow("drivable", drivable_on_original)

class Lane_Publisher():
    def __init__(self) -> None:
        self.lane_pub = rospy.Publisher("/lane_image", Image, queue_size=1)

pub_lanes = Lane_Publisher()


class Drivable_Area_Publisher():
    def __init__(self) -> None:
        self.drivable_pub = rospy.Publisher("/drivable_image", Image, queue_size=1)

pub_drivable = Drivable_Area_Publisher()


if __name__ == '__main__':
    rospy.init_node('hybridnets_ros', anonymous=True)
    detect_object = hybridnets_ros()
    rate = rospy.Rate(10)
    while not rospy.is_shutdown():
        seg_mask_=detect_object.detect()
        print(seg_mask_)
        if seg_mask_ is not None:
            detect_object.lane_publisher(seg_mask_)
            detect_object.drivable_publisher(seg_mask_)
            detect_object.lane_on_original(seg_mask_)
            detect_object.drivable_on_original(seg_mask_)
            cv2.waitKey(1)
        rate.sleep()