
Orin Devkit 支持4路 OX08B GMSL2 相机

说明：
1. 本文件为ORIN下的GMSL2相机驱动文件
2. 本驱动支持艾利光OX08B相机，4路同时出图。
3. 需要使用ORIN模组+ORIN原生开发板进行测试。（不支持Xavier底板）
4. 本固件为JetPack5.02版本
5. 若需要使用艾利光模组的4路点亮，需要进行PINMUX的更新，具体看刷机教程内容。

操作步骤：
step1： 拷贝本文件夹到任意的ORIN目录，并打开终端terminal
step2： chmod +x upgrade.sh install_ko.sh bring*
step3： ./upgrade.sh
step4： 重启ORIN，驱动即完成更新。
step5: 执行 ./install_ko.sh 进行驱动加载，等待加载完成。

相机预览命令：
cam0: gst-launch-1.0 v4l2src device=/dev/video0 ! 'video/x-raw,format=UYVY,width=3840,height=2160' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false
cam1: gst-launch-1.0 v4l2src device=/dev/video1 ! 'video/x-raw,format=UYVY,width=3840,height=2160' ! videoconvert ! fpsdisplaysink video-sink=xvimagesink sync=false
...

注意事项：
1. 相机不支持热插拔，在上电前，需要将相机连接好。
2. ORIN启动时会对连接的相机进行驱动加载，如需查看加载的驱动个数，在终端terminal 输入：
	
   ls | grep /dev/video* 
   
   连接了几个设备即会出现几个video 设备文件。

版本查看
uname -a


升级完成等待机器开机并查看内核和dtb日期是否已更新？ 如遇到功能异常，需提供如下命令的结果以便分析
dmesg > dmesg.log  （内核debug 日志）



