# 简介

此源码适用于[jetpack Jstson-linux:36.4.3](https://developer.nvidia.com/embedded/jetson-linux-r3643)


# 编译指导：

官方建议使用ubuntu20~ubuntu22去作为宿主机去编译

在宿主机linux建立一个工作区，比如如下,download放上面下载的bsp源码和交叉变工具链：

``` sh
➜  tree
.
├── bsp_src
├── download
├── kernel_src
└── tool
```


## 下载

首先进入Jstson-linux:36.4.3 官网链接：

https://developer.nvidia.com/embedded/jetson-linux-r3643


找到下方下载连接处的的BSP源码并下载到宿主机download目录：

[Driver Package (BSP) Sources](https://developer.nvidia.com/downloads/embedded/l4t/r36_release_v4.3/sources/public_sources.tbz2)

找到下方下载连接处的的教程编译工具链并下载到宿主机download目录：

[Bootlin Toolchain gcc 11.3](https://developer.nvidia.com/downloads/embedded/l4t/r36_release_v3.0/toolchain/aarch64--glibc--stable-2022.08-1.tar.bz2)

## 解压和编译

然后按照如下操作解压

``` sh

# 解压bsp源码
$ tar -xf download/public_sources.tbz2 -C bsp_src/

# 解压交叉编译工具链
$ tar -xf download/aarch64--glibc--stable-2022.08-1.tar.bz2   -C tool/

# 解压linux内核源码
$ tar -xf bsp_src/Linux_for_Tegra/source/kernel_src.tbz2 -C kernel_src/

# 解压树外模块源码
$ tar -xf bsp_src/Linux_for_Tegra/source/kernel_oot_modules_src.tbz2 -C kernel_src/

# 解压显示驱动源码
$ tar -xf bsp_src/Linux_for_Tegra/source/nvidia_kernel_display_driver_source.tbz2 -C kernel_src/

```

## 编译

``` sh

# 导出交叉编译工具链位置
$ export CROSS_COMPILE=$PWD/tool/aarch64--glibc--stable-2022.08-1/bin/aarch64-buildroot-linux-gnu-


# 导出内核源码位置
$ export KERNEL_HEADERS=$PWD/kernel_src/kernel/kernel-jammy-src


# 进入内核源码目录并运行编译脚本
$ cd kernel_src/
$ ./nvbuild.sh

```

## 编译自己的驱动

把我们自己的源码拷贝到解压的内核源码目录替换

然后重新编译，不需要要编译内核镜像的话，可以使用编译脚本加参数 -m只编译书外模块

``` sh
# 只编译树外模块
$ ./nvbuild.sh -m
``` 

