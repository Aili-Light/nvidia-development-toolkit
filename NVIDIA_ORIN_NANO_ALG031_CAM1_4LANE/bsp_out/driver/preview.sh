gst-launch-1.0 v4l2src device=/dev/video3 ! 'video/x-raw,format=UYVY,width=1920,height=1536' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false &
gst-launch-1.0 v4l2src device=/dev/video2 ! 'video/x-raw,format=UYVY,width=1920,height=1536' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false
