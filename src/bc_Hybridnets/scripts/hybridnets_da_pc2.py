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
import pdb
# from message_filters import ApproximateTimeSynchronizer, Subscriber
import pickle
import matplotlib.pyplot as plt
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

                drivable = np.zeros((self.height, self.width, 3), dtype=np.uint8)
                drivable[seg_mask_==1] = [0, 255, 0]

                drivable = cv2.resize(drivable, (self.cv_image.shape[1],self.cv_image.shape[0]))
                drivable = cv2.rotate(drivable, cv2.ROTATE_90_COUNTERCLOCKWISE)
                drivable = cv2.flip(drivable, 0)
  
                points = zip(list(np.where(drivable==255)[0]), list(np.where(drivable==255)[1]))


                self.header.stamp = rospy.Time.now()
                
                output_cloud = point_cloud2.create_cloud(self.header, self.fields, list(points))
                pub_drivable_pc2.drivable_pub.publish(output_cloud)
        for i in range(len(seg_mask_list)):
            for _, seg_mask in enumerate(seg_mask_list):

                seg_mask_ = seg_mask[i].squeeze().cpu().numpy()

                lanes = np.zeros((self.height, self.width, 3), dtype=np.uint8)
                # pdb.set_trace()
                lanes[seg_mask_==2] = [0, 255, 0]

                lanes_on_original = self.cv_image
                lanes_on_original=cv2.resize(lanes_on_original,(self.width,self.height))
                lanes_on_original[seg_mask_==2] = [0, 255, 0]
                # cv2.imshow("ll",lanes_on_original)
                # print("aaaaaaaaa")
                # s=(640,384)
                # empty_img=[]
                # empty_img.append(lanes_on_original)
                # print(empty_img)

                # cv2.waitKey(1)
                # gray = cv2.cvtColor(lanes_on_original,cv2.COLOR_BGR2GRAY)

                # # get coordinates (y,x) --- alternately see below for (x,y)
                # yx_coords = np.column_stack(np.where(gray >= 0))
                # print (yx_coords)

                # print ('')
                ###########################################################################################################
                objp = np.zeros((7*7,3), np.float32)
                objp[:,:2] = np.mgrid[0:7, 0:7].T.reshape(-1,2)

                # Arrays to store object points and image points from all the images.
                objpoints = [] # 3d points in real world space
                imgpoints = [] # 2d points in image plane.

              


                i=0
                while True:
                    i+=1
                    ret, img = cv2.imread(lanes_on_original)
                    # image = img
                    # Step through the list and search for chessboard corners
                    # for idx, fname in enumerate(images):
                    # img = cv2.imread(fname)
                    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
                    # Find the chessboard corners
                    ret, corners = cv2.findChessboardCorners(gray, (7,7), None)

                    # If found, add object points, image points
                    # cv2.waitKey(27)

                    # if ret == True:
                    objpoints.append(objp)
                    imgpoints.append(corners)

                    # Draw and display the corners
                    cv2.drawChessboardCorners(img, (7,7), corners, ret)
                    write_name = 'corners_found'+str(i)+'.jpg'
                    cv2.imwrite(write_name, img)
                    cv2.imshow('img', img)


                    if (cv2.waitKey(500) & 0xFF ==ord('q')):
                        cv2.destroyAllWindows()
                        break
                    img = cv2.imread('')
                    img_size = (img.shape[1], img.shape[0])
                    img.astype(list)


                    # Do camera calibration given object points and image points
                    ret, mtx, dist, rvecs, tvecs = cv2.calibrateCamera(objpoints, imgpoints, img_size,None,None)



                    dst = cv2.undistort(img, mtx, dist, None, mtx)
                    cv2.imwrite('calibration/corners_found2.jpg',dst)

                    # Save the camera calibration result for later use (we won't worry about rvecs / tvecs)
                    dist_pickle = {}
                    dist_pickle["mtx"] = mtx
                    dist_pickle["dist"] = dist
                    pickle.dump( dist_pickle, open( "calibration_wide/wide_dist_pickle.p", "wb" ) )
                    #dst = cv2.cvtColor(dst, cv2.COLOR_BGR2RGB)
                    # Visualize undistortion
                    f, (ax1, ax2) = plt.subplots(1, 2, figsize=(20,10))
                    ax1.imshow(img)
                    ax1.set_title('Original Image', fontsize=30)
                    ax2.imshow(dst)
                    ax2.set_title('Undistorted Image', fontsize=30)

                                    ###########################################################################################################calibration

                # # get coordinates (x,y)
                # xy_coords = np.flip(np.column_stack(np.where(gray >= 0)), axis=1)
                # print (xy_coords)
                if np.where(seg_mask_==1):
                    b=np.where(seg_mask_==1)

                a=[]
                
                a.append(b[0:1152][0:720])
                #print(a[0][1])
                # c=np.argmax(a)
                # print(c)
                #pdb.set_trace()
                y=a[0][0]
                x=a[0][1]
                p = np.poly1d( np.polyfit(x, y, 2) )

                plt.plot(x, -y, 'o', p, '-')
                plt.xlim(0, 1152)
                plt.ylim(-720,0)
                plt.show()
              
                    
                return 
     



class Drivable_pc2_pub():
    def __init__(self, pub_topic) -> None:
        self.drivable_pub = rospy.Publisher(
            pub_topic, PointCloud2, queue_size=1
        )

if __name__ == '__main__':
    rospy.init_node('hybridnets_ros', anonymous=True)
    global pub_drivable_pc2
    pub_drivable_pc2 = Drivable_pc2_pub(rospy.get_param("~pub_topic"))
    detect_object = hybridnets_ros()
    rate = rospy.Rate(10)
    while not rospy.is_shutdown():
        seg_mask_=detect_object.detect()
        rate.sleep()