# Support for Zedboard

## Warning 

Your warranty may become void. I am not responsible for bricked devices, dead SD cards, and any kind of problems in your device. 

The software is distributed with completely **no** warranty. 

## Zedboard

[Click here! ](http://zedboard.org/product/zedboard)

## Build
1. update `gcc` as
```apt install gcc-6-arm-linux-gnueabi```
2. cd ucore
3. export CROSS_COMPILE=arm-linux-gnueabi-
4. make ARCH=arm BOARD=zedboard defconfig
5. make sfsimg
6. make

## Run
TBD

## Environment

```
Arch Linux 
gcc 7.2.0 arm-none-eabi-gcc

```

## What works

Building and Goldfish emulation \(in [our environment](#development-environment)\)  
GPIO LED light  
Serial port (UART)  
Memory management  
Private clock

## What doesn't work
**Interrupt** (the program can't find the vector table)  
All others  


