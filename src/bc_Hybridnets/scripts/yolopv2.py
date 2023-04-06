#! /usr/bin/env python3


import rospy
import cv2
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
from yolopv2_model.utils import letterbox, split_for_trace_model, non_max_suppression, driving_area_mask, lane_line_mask, show_seg_result
import numpy as np
import torch

import pdb
class Detector():
    def __init__(self) -> None:

        self.bridge = CvBridge()
        self.cv_image = None
        self.device = torch.device(
            "cuda:0" if torch.cuda.is_available() else "cpu")
        # self.device = torch.device("cpu")
        self.weights = "/home/sevo/Lane/src/epoch-240.pth"
        # pdb.set_trace()
        self.stride = 32
        self.model = torch.jit.load(self.weights)

        # self.half = self.device.type != 'cpu'  # half precision only supported on CUDA
        self.model = self.model.to(self.device)

        self.imgsz = 640
        self.conf_thres = 0.3
        self.iou_thres = 0.45
        self.classes = None
        self.agnostic_nms = None

        if self.half:
            self.model.half()  # to FP16
        self.model.eval()

        if self.device.type != 'cpu':
            self.model(torch.zeros(1, 3, self.imgsz, self.imgsz).to(
                self.device).type_as(next(self.model.parameters())))  # run once

        rospy.Subscriber("/camera",
                         Image, self.callback, queue_size=1)

    def callback(self, input_image):
        self.cv_image = self.bridge.imgmsg_to_cv2(
            input_image, desired_encoding="")

    def detect(self):

        if self.cv_image is None:
            rospy.loginfo("no image received! :(")
            return

        img0 = cv2.resize(self.cv_image, (1280, 720),
                          interpolation=cv2.INTER_LINEAR)
        img = letterbox(img0, self.imgsz, stride=self.stride)[0]

        # Convert
        img = img[:, :, ::-1].transpose(2, 0, 1)  # BGR to RGB, to 3x416x416
        img = np.ascontiguousarray(img)

        img = torch.from_numpy(img).to(self.device)
        img = img.half() if self.half else img.float()  # uint8 to fp16/32
        img /= 255.0  # 0 - 255 to 0.0 - 1.0

        if img.ndimension() == 3:
            img = img.unsqueeze(0)

        [pred, anchor_grid], seg, ll = self.model(img)
        pred = split_for_trace_model(pred, anchor_grid)

        pred = non_max_suppression(
            pred, self.conf_thres, self.iou_thres, classes=self.classes, agnostic=self.agnostic_nms)

        da_seg_mask = driving_area_mask(seg)
        ll_seg_mask = lane_line_mask(ll)

        show_seg_result(img0, (da_seg_mask, ll_seg_mask), is_demo=True)
        drivable_lane = cv2.resize(
            img0, (self.cv_image.shape[1], self.cv_image.shape[0]))

        # drivable = np.zeros((da_seg_mask.shape[0], da_seg_mask.shape[1], 3), dtype=np.uint8) # değişken ismi 'img' olunca hata alıyor
        # drivable[da_seg_mask==1] = [0, 255, 0]
        # drivable[::2, ::2, :] = 0
        # drivable = cv2.resize(drivable, (self.cv_image.shape[1],self.cv_image.shape[0]))

        lane = np.zeros((ll_seg_mask.shape[0], ll_seg_mask.shape[1], 3), dtype=np.uint8) # değişken ismi 'img' olunca hata alıyor
        lane[ll_seg_mask==1] = [255, 0, 0]
        lane[::2, ::2, :] = 0
        lane = cv2.resize(lane, (self.cv_image.shape[1],self.cv_image.shape[0]))

        # cv2.imshow("anonim", drivable_lane)
        # cv2.waitKey(1)


def main():
    rospy.init_node("anonim", anonymous=False)
    detector = Detector()
    rate = rospy.Rate(20)
    while not rospy.is_shutdown():
        rate.sleep()
        detector.detect()


if __name__ == '__main__':

    main()
