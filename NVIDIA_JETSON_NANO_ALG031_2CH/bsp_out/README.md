# Driver Installation
[中文](./README_CN.md)

## 1 Prerequisites

This driver is ONLY works for Jetson Nano development kit.  

## 2 Driver files

Overview of driver files

``` bash
├── boot
│   ├── Image                                           # kernel image
│   ├── dtb
│       └── tegra210-p3448-0000-p3449-0000-b00.dtb      # device tress

├── README.md                                           # readme
└── driver
    ├── max9296_96717.ko                                # driver for deserdes
    ├── max96717.ko                                     # driver for serdes
    ├── alg031.ko                                       # driver for image sensor
    └── insmod.sh                                       # script to load driver
```

## 3 Update device tree

The driver must work with correct device tree. We have to update the device tree.  

### 3.1 Backup the old device tree

Check the location(/boot/dtb) to verify if the device tree file (kernel_tegra234-p3701-0004-p3737-0000.dtb) exists.  

If exists, backup the file:  

` sudo cp /boot/dtb/kernel_tegra234-p3701-0005-p3737-0000.dtb /boot/dtb/kernel_tegra234-p3701-0005-p3737-0000_backup.dtb  `

If not exists，backup device tree in /boot:  

` sudo cp /boot/tegra234-p3701-0005-p3737-0000.dtb /boot/dtb/kernel_tegra234-p3701-0005-p3737-0000_backup.dtb  `

### 3.2 Change the device tree

Update the device tree file on the jeston agx orin：

` sudo cp bsp_out/boot/dtb/kernel_tegra234-p3701-0005-p3737-0000.dtb /boot/dtb/kernel_tegra234-p3701-0005-p3737-0000.dtb `

### 3.4 Reboot and verify

Reboot. The system will use the UEFI to load /boot/extlinux/extlinux.conf and automatically update the device tree.   

After reboot, check if the device is loaded correcly.  

` ls /proc/device-tree/i2c@3180000/tca9543@72/i2c@0/ `

See if the following address appear:
`alg031_a@1b`、`alg031_b@1c`、`max9296_96717@48`、`max96717_a@42`、` max96717_b@44`、`max96717_prim@40`  
It means that device tree is loaded sucessfully.  

## 4 Load driver

### 4.1 Load driver

Copy ` bsp_out/driver ` to the jeston agx orin.  

Add permission to execute.   

` sudo chmod +x insmod.sh `

Load driver.  

` sudo ./insmod.sh `

If driver is loaded, we can see video devices on the position : /dev/video*

For example, if we have 4X3MP camera, we can see that:  

``` bash
$ ls /dev/video*
/dev/video0  /dev/video1  /dev/video2  /dev/video3
```

### 4.2 Preview

Use the command to preview image.  

` gst-launch-1.0 v4l2src device=/dev/video0 ! 'video/x-raw,format=UYVY,width=1920,height=1536' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false
 `


