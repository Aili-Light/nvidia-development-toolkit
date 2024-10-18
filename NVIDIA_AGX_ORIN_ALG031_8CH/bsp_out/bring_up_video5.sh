#!/bin/sh
gst-launch-1.0 v4l2src device=/dev/video5 ! 'video/x-raw,format=UYVY,width=1920,height=1536' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false
