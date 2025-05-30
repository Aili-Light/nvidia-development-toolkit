# GMSL Camera Adapter Kit Driver for NVIDIA Platform

This repository provides ALG-Tech's GMSL camera driver framework for the NVIDIA Jetson AGX Orin platform. The driver is only compatible with ALG-Tech's GMSL camera adapter board. [中文](./README_CN.md)

For more information check the [website](https://www.alg-imaging.com)

## Important Note

This driver is compatible with `NVIDIA JETSON AGX ORIN Development Kit 64G`

<img src="./NVIDIA_AGX_ORIN_ALG_CAM_COMM_8CH/doc/pic/jetson-orin-autonomous-machines-overview-og.jpg" alt="pic6" style="zoom:50%;" />

Make sure the following version numbers are matched.  
1. Jetpack Version <6.2>
2. Jetson Linux Version <r36.4.3>

Verify the information by using `jtop` command on your device.

![image-20250418194318082](./NVIDIA_AGX_ORIN_ALG_CAM_COMM_8CH/doc/pic/pic7.png)

## Prerequisites
1. ALG GMSL Camera (https://www.alg-imaging.com/product/alg-3m-automotive-camera-isx031-driving-front-view-adas-hd-waterproof-gmsl2/)  

2. ALG GMSL Camera Adapter Board(https://www.alg-imaging.com/product/alg-8-channel-gmsl-adapter-board-for-jetson-orin-development-kit/)  

# Hardware setup
## Get ready for hardwares  
- NVIDIA Jetson AGX ORIN Official Development Kit  
- 12V Power adapter  
- ALG GMSL adapter board  
- ALG GMSL camera module (3MP/8MP)

## Introduction to the Jetson ORIN Adapter Board
1. Main board  
![main board front](./NVIDIA_AGX_ORIN_ALG_CAM_COMM_8CH/doc/pic/image-1.png)
![main board back](./NVIDIA_AGX_ORIN_ALG_CAM_COMM_8CH/doc/pic/image-2.png)

2. Sub board  
![sub board](./NVIDIA_AGX_ORIN_ALG_CAM_COMM_8CH/doc/pic/image.png)

## Setup instruction
1. Stackup main board on the connector at the back side of Jetson Orin.  
2. Connect main board with two sub boards by FPC cables. 
3. Connect power cable to the power port(12V) of main board.  
4. Connect GMSL cameras to the Fakra connector on sub board (maximum 4-CH per board).  

## Hardware setup as displayed in the following image  
![hardware setup(back)](./NVIDIA_AGX_ORIN_ALG_CAM_COMM_8CH/doc/pic/image-4.png)
![hardware setup(top)](./NVIDIA_AGX_ORIN_ALG_CAM_COMM_8CH/doc/pic/image-5.png)

# Camera Driver
Please refer to [Driver](./NVIDIA_AGX_ORIN_ALG_CAM_COMM_8CH/doc/README.md) for specific descriptions.

# Support
contact : jimmy@alg-imaging.com

# Copyright
2020-2025 Shenzhen Aili-Light Co.,Ltd  