#!/bin/sh
gst-launch-1.0 v4l2src device=/dev/video0 ! 'video/x-raw,format=YUY2,width=1280,height=800' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false
