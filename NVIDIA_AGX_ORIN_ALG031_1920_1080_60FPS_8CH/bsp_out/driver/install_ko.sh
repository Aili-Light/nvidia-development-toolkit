#!/bin/bash

sudo insmod max9296_96717.ko
sudo insmod max96717.ko
sudo insmod nv_alg031.ko


echo 1 > /sys/kernel/debug/bpmp/debug/clk/vi/mrq_rate_locked
echo 1 > /sys/kernel/debug/bpmp/debug/clk/isp/mrq_rate_locked
echo 1 > /sys/kernel/debug/bpmp/debug/clk/nvcsi/mrq_rate_locked










