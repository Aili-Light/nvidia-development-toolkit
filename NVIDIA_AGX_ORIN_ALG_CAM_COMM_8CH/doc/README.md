# NVIDIA JETSON AGX ORIN Camera Driver Framework

The ALG's camera driver framework for NVIDIA Jetson AGX Orin platform. 

## 1. File Descriptions

``` sh
├── app
│   ├── config
│   │   ├── alg_isx_031_1_ch_master.json        # alg031 master single channel configuration
│   │   ├── alg_isx_031_2_ch_master.json        # alg031 master dual channel configuration
│   │   └── table
│   │       ├── ALG_MADE_0003101_9296_96717_6G_ISX031_YUV422_1920_1536_MIPI_4LANES_30FPS_MFP9_Master_AFL2_CH_V2_0.txt
│   │       └── ALG_MADE_0103101_9296_96717_6G_ISX031_YUV422_1920_1536_MIPI_4LANES_30FPS_MFP9_Master_AFL1_CH_V2_0.txt
│   └── config_sensor_test              # Camera configuration tool
├── boot
│   └── tegra234-eac5000-camera-aili-light-max9296-overlay.dtbo # device tree file
├── doc
│   └── readme.md                       # Readme
└── driver
│   └── alg-max9296.ko                  # Driver file
└── 8ch_031_master_test.sh              # 8-CH bringup script
```


## 2. Flash system and change pinmux

Flash the jeston linux system to version <r36.4.3> according to the official instruction. 
​	https://docs.nvidia.com/jetson/archives/r36.4.3/DeveloperGuide/IN/QuickStart.html#preparing-a-jetson-developer-kit-for-use

Replace the pinmux and gpio device tree file by the files provided in the bsp folder.  

```sh
└── bootloader
    ├── BCT
    │   └── tegra234-mb1-bct-pinmux-p3701-0000-a04.dtsi
    └── tegra234-mb1-bct-gpio-p3701-0000-a04.dtsi
```


## 3. Device Tree Overlay

Note : The device tree overlay only need to be done once. It is not required to do this operation again after reboot.

Please refer to the official instruction.

[Configuring the CSI Connector](https://docs.nvidia.com/jetson/archives/r36.4.3/DeveloperGuide/HR/ConfiguringTheJetsonExpansionHeaders.html#configuring-the-csi-connector)


1. Run command `sudo cp boot/tegra234-eac5000-camera-aili-light-max9296-overlay.dtbo /boot/`. It will place the device tree file under folder `/boot`.

2. Run command `sudo /opt/nvidia/jetson-io/jetson-io.py` and open app <jetson-io>

3. Select `Configure Jetson AGX CSI Connector`

![pic1](./pic/pic1.png)

4. Select `Configure for compatible hardware`

![pic2](./pic/pic2.png)

5. Select `AGX ORIN Aili-Light MAX9296A Device Tree Overlay`

![pic3](./pic/pic3.png)

6. Select `Save pin changes`

![pic4](./pic/pic4.png)

7. Select `Save and reboot to reconfigure pins`

![pic5](./pic/pic5.png)

8. After rebooting, check if the device tree overlay is done successfully.  
``` sh
$ ls /proc/device-tree/bus@0/i2c@3180000/tca9546@70/i2c@0/

gmslcomm_a@4 gmslcomm_b@5 ...
```

## 4. Load Camera Driver

Every time after booting the system, it is required to load the camera driver `alg-max9296.ko` mannually.  

`sudo insmod driver/alg-max9296.ko`

Note: it will take 3-5 seconds to complete the operation.
User can check if the driver is loaded by runing the command.
``` sh
$ ls /dev/vid*
```
If the following device nodes are visible, it means the driver is loaded sucessfully.
```
/dev/video0  /dev/video1  /dev/video2  /dev/video3  /dev/video4  /dev/video5  /dev/video6  /dev/video7
```



## 5. Load Driver for Trigger

After booting the system, it is required to load the driver for trigger `trigger.ko` mannually.

`sudo insmod driver/trigger.ko`

User can check if the driver is loaded by runing the command.
``` sh
$ ls /dev/aili_trigger*
``` 
If the following device nodes are visible, it means the driver is loaded sucessfully.
``` 
/dev/aili_trigger0
```



## 6. Configurate Camera

Use the app `config_sensor_test` to load the camera configuration.  

Important Note: each two adjacent channels, starting from 0 to 7, are shared with a same deserailizer, and the driver can not support two different types of cameras on the same deserializer.

For example, if two 3MP ALI031 cameras are connected on channel 0 and channel 1, user can send dual-channel configuration file `alg_isx_031_2_ch_master.json` to either channel 0 or channel 1. However, if only one ALI031 camera is connected to either channel 0 or channel 1, user need to send single-channel configuration file `alg_isx_031_1_ch_master.json` to the designated channel, and can only get video output from channel 1 (device note : video1).  

For 8 channel ALI031 camera, run the following command.  
``` sh
$ cd app

$ ./config_sensor_test --set-ch-param --type MAX9296A --ch 0  --param ./config/alg_isx_031_2_ch_master.json

sleep 3s

$ ./config_sensor_test --set-ch-param --type MAX9296A --ch 2  --param ./config/alg_isx_031_2_ch_master.json

sleep 3s

$ ./config_sensor_test --set-ch-param --type MAX9296A --ch 4  --param ./config/alg_isx_031_2_ch_master.json

sleep 3s

$ ./config_sensor_test --set-ch-param --type MAX9296A --ch 6  --param ./config/alg_isx_031_2_ch_master.json

```

For 4 channel ALIX8B camera, run the following command.  
``` sh
$ cd app

$ ./config_sensor_test --set-ch-param --type MAX9296A --ch 0  --param ./config/alg_ox08b_2_ch_96717_master.json

sleep 3s

$ ./config_sensor_test --set-ch-param --type MAX9296A --ch 2  --param ./config/alg_ox08b_2_ch_96717_master.json

sleep 3s

$ ./config_sensor_test --set-ch-param --type MAX9296A --ch 4  --param ./config/alg_ox08b_2_ch_96717_master.json

sleep 3s

$ ./config_sensor_test --set-ch-param --type MAX9296A --ch 6  --param ./config/alg_ox08b_2_ch_96717_master.json

```



## 7. Trigger Configuration

Internal trigger mode (30Hz), run the following two commands.  
```sh
sudo ./aili_trigger_test --set_param 2
sudo ./aili_trigger_test --status on
```

External trigger mode, run the following two commands.  
```sh
sudo ./aili_trigger_test --set_param 0
sudo ./aili_trigger_test --status on
```



## 8. Speed Up

The Jetson AGX Orin will need to speed up if connected with two or more 8MP cameras (ALIX8B).
``` sh
# Increase frequency for vi/isp/nvcsi 
echo 1 | sudo tee /sys/kernel/debug/bpmp/debug/clk/vi/mrq_rate_locked
echo 1 | sudo tee /sys/kernel/debug/bpmp/debug/clk/isp/mrq_rate_locked
echo 1 | sudo tee /sys/kernel/debug/bpmp/debug/clk/nvcsi/mrq_rate_locked
echo 1 | sudo tee /sys/kernel/debug/bpmp/debug/clk/emc/mrq_rate_locked
sudo cat /sys/kernel/debug/bpmp/debug/clk/vi/max_rate    | sudo tee /sys/kernel/debug/bpmp/debug/clk/vi/rate
sudo cat /sys/kernel/debug/bpmp/debug/clk/isp/max_rate   | sudo tee /sys/kernel/debug/bpmp/debug/clk/isp/rate
sudo cat /sys/kernel/debug/bpmp/debug/clk/nvcsi/max_rate | sudo tee /sys/kernel/debug/bpmp/debug/clk/nvcsi/rate
sudo cat /sys/kernel/debug/bpmp/debug/clk/emc/max_rate   | sudo tee /sys/kernel/debug/bpmp/debug/clk/emc/rate
```


## 9. Preview

Use gstreamer for preview. For example, to display videos for 8 channel 3MP ALI031 camera.
``` sh
$ gst-launch-1.0 v4l2src device=/dev/video0 ! 'video/x-raw,format=UYVY,width=1920,height=1536' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false &
$ gst-launch-1.0 v4l2src device=/dev/video1 ! 'video/x-raw,format=UYVY,width=1920,height=1536' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false &
$ gst-launch-1.0 v4l2src device=/dev/video2 ! 'video/x-raw,format=UYVY,width=1920,height=1536' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false &
$ gst-launch-1.0 v4l2src device=/dev/video3 ! 'video/x-raw,format=UYVY,width=1920,height=1536' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false &
$ gst-launch-1.0 v4l2src device=/dev/video4 ! 'video/x-raw,format=UYVY,width=1920,height=1536' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false &
$ gst-launch-1.0 v4l2src device=/dev/video5 ! 'video/x-raw,format=UYVY,width=1920,height=1536' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false &
$ gst-launch-1.0 v4l2src device=/dev/video6 ! 'video/x-raw,format=UYVY,width=1920,height=1536' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false &
$ gst-launch-1.0 v4l2src device=/dev/video7 ! 'video/x-raw,format=UYVY,width=1920,height=1536' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false &
```

Display videos for 8 channel 8MP ALIX8B camera.
``` sh
$ gst-launch-1.0 v4l2src device=/dev/video0 ! 'video/x-raw,format=UYVY,width=3840,height=2160' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false &
$ gst-launch-1.0 v4l2src device=/dev/video1 ! 'video/x-raw,format=UYVY,width=3840,height=2160' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false &
$ gst-launch-1.0 v4l2src device=/dev/video2 ! 'video/x-raw,format=UYVY,width=3840,height=2160' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false &
$ gst-launch-1.0 v4l2src device=/dev/video3 ! 'video/x-raw,format=UYVY,width=3840,height=2160' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false &
$ gst-launch-1.0 v4l2src device=/dev/video4 ! 'video/x-raw,format=UYVY,width=3840,height=2160' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false &
$ gst-launch-1.0 v4l2src device=/dev/video5 ! 'video/x-raw,format=UYVY,width=3840,height=2160' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false &
$ gst-launch-1.0 v4l2src device=/dev/video6 ! 'video/x-raw,format=UYVY,width=3840,height=2160' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false &
$ gst-launch-1.0 v4l2src device=/dev/video7 ! 'video/x-raw,format=UYVY,width=3840,height=2160' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false &
```