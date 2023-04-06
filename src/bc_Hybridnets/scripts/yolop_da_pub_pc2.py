#! /usr/bin/env python3

from yolop_model.utils.augmentations import letterbox_for_img
from yolop_model.models import get_net
from yolop_model.utils.utils import create_logger, select_device
from yolop_model.config import cfg
import torchvision.transforms as transforms
import numpy as np
import torch
import cv2
import os
import shutil


normalize = transforms.Normalize(
    mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225]
)

transform = transforms.Compose([
    transforms.ToTensor(),
    normalize,
])

from sensor_msgs import point_cloud2
from std_msgs.msg import Header
from sensor_msgs.msg import PointCloud2, PointField
from cv_bridge import CvBridge
from sensor_msgs.msg import Image
import rospy

class Detect():

    def __init__(self, cfg) -> None:

        self.cfg = cfg

        self.weight = rospy.get_param('~yolop_weight')
        self.source = 'YOLOP/inference/videos'
        self.img_size = rospy.get_param('~img_size')
        self.conf_thres = rospy.get_param('~conf_thres')
        self.iou_thres = rospy.get_param('~iou_thres')
        self.device_name = rospy.get_param('~device')
        self.save_dir = rospy.get_param('~save_dir')

        self.device = None
        self.model = None
        self.cv_image = None

        self.header = Header()
        self.counter = 0
        self.header.frame_id = None
        self.fields = [PointField('x', 0, PointField.FLOAT32, 1),
                       PointField('y', 4, PointField.FLOAT32, 1)]

        self.Model(self.cfg)
        rospy.Subscriber(rospy.get_param('~image_topic'), Image, self.callback, queue_size=1)

    def Model(self, cfg):
        logger, _, _ = create_logger(cfg, cfg.LOG_DIR, 'bc_lane_demo')

        self.device = select_device(logger, self.device_name)
        if os.path.exists(self.save_dir):
            shutil.rmtree(self.save_dir)
        # os.mkdirs(self.save_dir)
        half = self.device.type != 'cpu'

        # Load model
        self.model = get_net(cfg)
        checkpoint = torch.load(self.weight, map_location=self.device)
        self.model.load_state_dict(checkpoint['state_dict'])
        self.model = self.model.to(self.device)
        if half:
            self.model.half()

        # Get names and colors

        img = torch.zeros((1, 3, self.img_size, self.img_size),
                          device=self.device)  # init image
        _ = self.model(img.half() if half else img) if self.device.type != 'cpu' else None
        self.model.eval()

    def callback(self, input_image):

        bridge = CvBridge()
        self.cv_image = bridge.imgmsg_to_cv2(input_image, desired_encoding="")

    def detect_lane(self):
        if self.cv_image is None:
            return

        h_cv, w_cv = self.cv_image.shape[:2]

        img, ratio, pad = letterbox_for_img(self.cv_image, self.img_size)
        h_img, w_img = img.shape[:2]
        shapes = (h_cv, w_cv), ((h_img / h_cv, w_img / w_cv), pad)

        half = self.device != 'cpu'

        img = transform(img).to(self.device)
        img = img.half() if half else img.float()
        if img.ndimension() == 3:
            img = img.unsqueeze(0)

        det_out, da_seg_out, ll_seg_out = self.model(img)

        _, _, height, width = img.shape
        h, w, _ = self.cv_image.shape
        pad_w, pad_h = shapes[1][1]
        pad_w = int(pad_w)
        pad_h = int(pad_h)
        ratio = shapes[1][0][1]

        da_predict = da_seg_out[:, :, pad_h:(height-pad_h), pad_w:(width-pad_w)]
        da_seg_mask = torch.nn.functional.interpolate(da_predict, scale_factor=int(1/ratio), mode='bilinear')
        _, da_seg_mask = torch.max(da_seg_mask, 1)
        da_seg_mask = da_seg_mask.int().squeeze().cpu().numpy()
        
        drivable = np.zeros((da_seg_mask.shape[0], da_seg_mask.shape[1], 3), dtype=np.uint8) # değişken ismi 'img' olunca hata alıyor
        drivable[da_seg_mask==1] = [0, 255, 0]
        drivable[::2, ::2, :] = 0

        drivable = cv2.resize(drivable, (self.cv_image.shape[1],self.cv_image.shape[0]))
        drivable = cv2.rotate(drivable, cv2.ROTATE_90_COUNTERCLOCKWISE)
        drivable = cv2.flip(drivable, 0)

        points = zip(list(np.where(drivable==255)[0]), list(np.where(drivable==255)[1]))

        self.header.frame_id = str(self.counter)
        self.counter += 1
        self.header.stamp = rospy.Time.now()
        output_cloud = point_cloud2.create_cloud(self.header, self.fields, list(points))

        print('Detected -------')

        lane_publisher.lane_pub.publish(output_cloud)

class Lane_Publisher():
    def __init__(self) -> None:
        self.lane_pub = rospy.Publisher(
            "/lane_2d", PointCloud2, queue_size=1)


lane_publisher = Lane_Publisher()


def main():

    rospy.init_node("lane_publisher_pc2", anonymous=False)

    detect = Detect(cfg)

    rate = rospy.Rate(20)
    while not rospy.is_shutdown():
        detect.detect_lane()
        rate.sleep()
        
if __name__ == '__main__':

    main()