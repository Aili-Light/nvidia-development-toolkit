# 驱动安装说明

## 1 前提

本驱动适用于nvidia官方的 jeston agx orin 平台，

基于官方R35.5的内核源码上编译生成。


## 2 文件介绍

此驱动主要提供如下的文件：

``` bash
├── boot
│   ├── Image                                           # 内核镜像
│   ├── dtb
│   │    ├── SENSOR_ADDR_0X80
│   │       └── kernel_tegra234-p3767-0003-p3768-0000-a0.dtb   # SNSOR地址为0x80设备树
│   │    ├── SENSOR_ADDR_0X84
│   │       └── kernel_tegra234-p3767-0003-p3768-0000-a0.dtb   # SNSOR地址为0x84设备树设备树
│   └── extlinux
│       └── extlinux.conf                               # 启动配置文件
├── README.md                                           # 使用说明
└── driver
    ├── install_ko.sh                                   # 驱动安装脚本
    ├── max9296_96717.ko                                # 解串器驱动
    ├── max96717.ko                                     # 串行器驱动
    ├── nv_alg031.ko                                    # 摄像头驱动
    └── preview.sh                                      # 8通道预览脚本
```


## 3 更新设备树

驱动需要和设备树一起使用，所以需要先更细设备树。

### 3.1 备份旧的设备树

首先查看 /boot/dtb下是否存在kernel_tegra234-p3767-0003-p3768-0000-a0.dtb

如果存在,备份这个设备树

` sudo cp /boot/dtb/kkernel_tegra234-p3767-0003-p3768-0000-a0.dtb /boot/dtb/kkernel_tegra234-p3767-0003-p3768-0000-a0.dtb_backup.dtb  `

如果不存在，备份/boot 文件夹下的设备树

` sudo cp /boot/tegra234-p3767-0003-p3768-0000-a0.dtb.dtb /boot/dtb/kernel_tegra234-p3767-0003-p3768-0000-a0.dtb_backup.dtb  `

### 3.2 修改启动配置

备份配置文件/boot/extlinux/extlinux.conf

` sudo cp /boot/extlinux/extlinux.conf /boot/extlinux/extlinux.conf.backup `

使用此文件夹中的配置文件，更新jeston agx orin中的配置文件

` sudo cp bsp_out/boot/extlinux/extlinux.conf  /boot/extlinux/extlinux.conf `

### 3.3 修改设备树

使用此文件夹中的设备树，更新jeston agx orin中的设备树：

` sudo cp bsp_out/boot/dtb/kernel_tegra234-p3767-0003-p3768-0000-a0.dtb /boot/dtb/kernel_tegra234-p3767-0003-p3768-0000-a0.dtb `

### 3.4 验证设备树更新成功

重新启动一下系统，让uefi加载配置/boot/extlinux/extlinux.conf，让系统加载新的设备树。

系统启动之后，可以通过如下命令查看一下摄像头节点是否存在，

` ls  /proc/device-tree/cam_i2cmux/i2c@0/ `

sensor默认地址为0x80时可以看到
`alg031_a@1b`、`alg031_b@1c`、`max9296_96717@48`、`max96717_a@42`、` max96717_b@44`、`max96717_prim@40`
默认地址为0x84是，可以看到
`alg031_a@1b`、`alg031_b@1c`、`max9296_96717@48`、`max96717_a@40`、` max96717_b@44`、`max96717_prim@42`
这些节点，表示设备树加载正常，否则设备树更新失败。


## 4 加载驱动

### 4.1 加载驱动

将` bsp_out/driver `文件夹的文件驱动拷贝到jeston agx orin中


增加执行权限

` sudo chmod +x install_ko.sh `

加载驱动

` sudo ./install_ko.sh `

这里如果加载成功的话，可以在看到video设备：/dev/video*

比如接了4 个alg031个摄像头,可以看到：

``` bash
$ ls /dev/video*
/dev/video0  /dev/video1  /dev/video2  /dev/video3
```

### 4.2 预览

可以使用如下指令预览视频，device后的参数是video的设备节点

` gst-launch-1.0 v4l2src device=/dev/video0 ! 'video/x-raw,format=UYVY,width=1920,height=1536' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false
 `

