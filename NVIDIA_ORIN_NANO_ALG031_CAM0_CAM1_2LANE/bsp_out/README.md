# Driver Installation
[中文](./README_CN.md)

## 1 Prerequisites

This driver is ONLY works for jeston orin nano with kernel R35.5

## 2 Driver files

Overview of driver files

``` bash
├── boot
│   ├── Image                                           # kernel image
│   ├── dtb
│   │    ├── SENSOR_ADDR_0X80
│   │       └── kernel_tegra234-p3767-0003-p3768-0000-a0.dtb   # device tree (sensor addr 0x80)
│   │    ├── SENSOR_ADDR_0X84
│   │       └── kernel_tegra234-p3767-0003-p3768-0000-a0.dtb   # device tree (sensor addr 0x84)
│   └── extlinux
│       └── extlinux.conf                               # boot configuration
├── README.md                                           # readme
└── driver
    ├── install_ko.sh                                   # script for driver installation 
    ├── max9296_96717.ko                                # driver for deserdes
    ├── max96717.ko                                     # driver for serdes
    ├── nv_alg031.ko                                    # driver for image sensor
    └── preview.sh                                      # script for preview
```
## 3 Update device tree

The driver must work with correct device tree. We have to update the device tree.  

### 3.1 Backup the old device tree

Check the location(/boot/dtb) to verify if the device tree file (kernel_tegra234-p3767-0003-p3768-0000-a0.dtb) exists.  

If exists, backup the file:  

` sudo cp /boot/dtb/kkernel_tegra234-p3767-0003-p3768-0000-a0.dtb /boot/dtb/kkernel_tegra234-p3767-0003-p3768-0000-a0.dtb_backup.dtb  `

If not exists，backup device tree in /boot:  

` sudo cp /boot/tegra234-p3767-0003-p3768-0000-a0.dtb.dtb /boot/dtb/kernel_tegra234-p3767-0003-p3768-0000-a0.dtb_backup.dtb  `

### 3.2 Modify the boot configuration

Backup config file : /boot/extlinux/extlinux.conf

` sudo cp /boot/extlinux/extlinux.conf /boot/extlinux/extlinux.conf.backup `

Update configuration file on the jeston orin nano  

` sudo cp bsp_out/boot/extlinux/extlinux.conf  /boot/extlinux/extlinux.conf `

### 3.3 Change the device tree

Update the device tree file on the jeston orin nano  

` sudo cp bsp_out/boot/dtb/kernel_tegra234-p3767-0003-p3768-0000-a0.dtb /boot/dtb/kernel_tegra234-p3767-0003-p3768-0000-a0.dtb `

### 3.4 Reboot and verify

Reboot. The system will use the UEFI to load /boot/extlinux/extlinux.conf and automatically update the device tree.   

After reboot, check if the device is loaded correcly.  

` ls  /proc/device-tree/cam_i2cmux/i2c@0/ `

See the following address appear if sensor addr is 0x80  
`alg031_a@1b`、`alg031_b@1c`、`max9296_96717@48`、`max96717_a@42`、` max96717_b@44`、`max96717_prim@40`
See the following address appear if sensor addr is 0x84
`alg031_a@1b`、`alg031_b@1c`、`max9296_96717@48`、`max96717_a@40`、` max96717_b@44`、`max96717_prim@42`
It means that device tree is loaded sucessfully.  

## 4 Load driver

### 4.1 Load driver

Copy` bsp_out/driver `to the jeston orin nano  

Add permission to execute.   

` sudo chmod +x install_ko.sh `

Load driver.  

` sudo ./install_ko.sh `

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
