#!/bin/bash
cd "$(dirname "$0")"


sudo /sbin/insmod ../module_main.ko
sudo ./test_main
