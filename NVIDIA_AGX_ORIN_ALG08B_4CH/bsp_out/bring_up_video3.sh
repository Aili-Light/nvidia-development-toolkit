#!/bin/sh
gst-launch-1.0 v4l2src device=/dev/video3 ! 'video/x-raw,format=UYVY,width=3840,height=2160' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false
