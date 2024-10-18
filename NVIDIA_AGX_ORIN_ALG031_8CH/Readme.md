Jetson Orin Adapter Board Driver for 8-CH ALI031A2(ISX031) Camera
====================================  

# Introductions
1. This is the GMSL2 camera driver for Jetson Orin Development Kit.  
2. The driver is for maximum 8-channels ALG-ALI031A2 camera.  
3. The driver works with the official version of Jetson Orin Development Kit.  
4. The driver is compatible with JetPack5.02.  
5. PINMUX update is required to connect ALG camera module.  

# Usuage
### Copy all the files to Jetson Orin and open the terminal

### Make the bash script executable
```bash
chmod +x upgrade.sh install_ko.sh
```
### Run update script
```bash
./upgrade.sh
```
### Reboot Orin to finsh

### Load the driver
```bash
./install_ko.sh
```
### Preview images
cam0:
```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! 'video/x-raw,format=UYVY,width=1920,height=1536' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false
```
cam1: 
```bash
gst-launch-1.0 v4l2src device=/dev/video1 ! 'video/x-raw,format=UYVY,width=1920,height=1536' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false
```
...

# Note
1. The camera driver is not hot pluggable. User must connect the camera correctly before power on.  
2. Jetson Orin will load camera drivers automatically on boot. To verify the driver loaded on the system, please use : 
```bash
ls | grep /dev/video* 
```
