
from __future__ import print_function

import numpy as np
import argparse
import cv2
import os 
import sys

sys.path.insert(0, '/Users/developer/guru/')

from utility import basics
from fileOp.imgReader import ImageReader

imgReader = ImageReader('./cup', False)

cntFrame = 0
t0 = cv2.getTickCount()
while (True):
	ret, img, imageName = imgReader.read()
	if (ret == False):
		break

	gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
	thresholded = basics.threshold_img(gray, '128')
	contour = cv2.findContours(thresholded, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
	contour = contour[1]

	drawing = np.zeros(img.shape, dtype=np.uint8)
	cv2.drawContours(drawing, contour, -1, (255, 0, 0), 2)
	name = 'output/pimg'+str(cntFrame)+'.png'
	cntFrame = cntFrame + 1
	cv2.imwrite(name, drawing)
t1 = cv2.getTickCount()
time = (t1 - t0)/ cv2.getTickFrequency()
print('---> take {}'.format(time))