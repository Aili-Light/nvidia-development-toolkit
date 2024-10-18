#!/bin/bash
sudo cp $PWD/Image /boot/
sudo cp $PWD/tegra234-p3701-0004-p3737-0000.dtb $PWD/kernel_tegra234-p3701-0004-p3737-0000.dtb
sudo cp $PWD/kernel_tegra234-p3701-0004-p3737-0000.dtb /boot/dtb/
sudo cp $PWD/extlinux.conf /boot/extlinux/ 

echo 'Upgrade FW success, please press Enter to reboot Jetson AGX Orin'
read key

sudo reboot
