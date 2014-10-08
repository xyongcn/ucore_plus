#########################################################################
# File Name: compile.sh
# Author: ma6174
# mail: ma6174@163.com
# Created Time: 2014年10月08日 星期三 16时40分44秒
#########################################################################
#!/bin/bash

make clean
make ARCH=i386 defconfig
make 
make sfsimg
./uCore_run
