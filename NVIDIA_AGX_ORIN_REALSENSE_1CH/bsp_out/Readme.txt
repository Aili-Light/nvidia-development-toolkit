说明：
1. 本文件为ORIN下的D457 GMSL2相机驱动文件
2. 本驱动支持D457相机，默认D457 RGB模式出图。
3. 本固件为JP5.02版本

操作步骤：
step1： 拷贝本文件夹到任意的ORIN目录，并打开终端terminal
step2： chmod +x upgrade.sh install_ko.sh bring_up_video0.sh
step3： ./upgrade.sh
step4： 重启ORIN，驱动即完成更新。
step5:	./install_ko.sh

相机预览命令：
./bring_up_video0.sh

注意事项：
1. 相机不支持热插拔，在上电前，需要将相机连接好。


版本查看
uname -a


升级完成等待机器开机并查看内核和dtb日期是否已更新？ 如遇到功能异常，需提供如下命令的结果以便分析
dmesg > dmesg.log  （内核debug 日志）



