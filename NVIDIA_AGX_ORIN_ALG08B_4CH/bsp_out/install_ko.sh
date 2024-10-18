#!/bin/sh
sudo insmod max9296_96717.ko
sudo insmod max96717.ko
sudo insmod nv_alg08b.ko
echo 'insmod ko success'

#echo 'input key to start streaming'
#read key
#gst-launch-1.0 v4l2src device=/dev/video2 ! 'video/x-raw,format=UYVY,width=1920,height=1536' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false

#gst-launch-1.0 v4l2src device=/dev/video1 ! 'video/x-raw,format=UYVY,width=1920,height=1536' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false
#v4l2-ctl -d /dev/video0 --set-fmt-video=width=1920 --set-ctrl bypass_mode=0 --stream-mmap --stream-count=0
