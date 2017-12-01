uCore+
==========

[![Travis CI status](https://travis-ci.org/oscourse-tsinghua/ucore_plus.svg?branch=master)](https://travis-ci.org/oscourse-tsinghua/ucore_plus?branch=master)

You have found uCore+, a highly experimental operating system intended for Operating System Course in Tsinghua University.

## Compile & Run

### System requirements
uCore+ supports a lot of different architectures, including x86, amd64, ARM and MIPS.

Compiling uCore+ requires you having a (cross) compiler  installed for the a specific architecture. For most  linux distributions, **multilib gcc** (supporting x86 and amd64), and **arm-none-eabi-gcc** have builtin packages. You may need to compile a **mips-sde-elf-gcc** yourself. It is suggested to use the exact architecture tuple recommanded here.

There are some other packages you need to install (below is there package name in Ubuntu):

* **ncurses-dev** : used to build uCore+ build configuration tools.
* **qemu** : Provide the virtualized environment to run uCore+. Note that to use QEMU binaries for ARM and MIPS, extra packages needs to be installed manually for some linux distributions.
* **u-boot-tools** : providing the **mkimage** command, used to create u-boot image for ARM uCore+.

## Quick Try
Run the following commands in your terminal to get uCore+ AMD64 running on QEMU.

 1. download or clone uCore+ source code
 1. cd ucore
 1. make ARCH=amd64 defconfig
 1. make
 1. make sfsimg
 1. ./uCore_run

## Build with buildroot image

## Current Progress
 We are working on ucore plus for amd64 smp porting.
 You can chekout the "amd64-smp" branch to see the newest progress of ucore plus.

## Compile & Run on ARM
 Refer to `README_ZED.md`.


## Makefile
**Cross Compile**

set the environment variables:

export ARCH = ?

you can use archs: i386, arm, amd64, mips, or32, um, nios2

export CROSS\_COMPILE = ?

(see Makefile in ./ucore)

**Kconfig**

The top level Kconfig is ./ucore/arch/xxx/Kconfig. You can include other Kconfig

  (see ./ucore/arch/arm/Kconfig)

  All config option is prefixed with UCONFIG_

**Makefile**

Supported variables:  obj-y, dirs-y

(See ./ucore/kern-ucore/Makefile.build and Makefile.subdir)

**Add a new ARCH**

In arch/xxx, add Makefile.image and include.mk

***include.mk: define ARCH_INCLUDES and ARCH_CFLAGD, etc.

***Makefile.image: how to generate the executable for your platform.

***Kconfig: your top level Kconfig

***Makefile: recursively add Makefile in your arch directory.

More Document
========
See ucore/doc
