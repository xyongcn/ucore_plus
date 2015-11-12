/*
 * =====================================================================================
 *
 *       Filename:  board-zedboard.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/12/2015 11:19:11 AM
 *       Revision:  none
 *       Compiler:  arm-linux-gcc 4.4
 *
 *         Author:  Chen Tianyu
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifndef  MACH_BOARD_ZEDBOARD_H
#define  MACH_BOARD_ZEDBOARD_H

// uart 0
#define ZEDBOARD_UART0      0xe0000000

// timer address
#define ZEDBOARD_TIMER_BASE 0xf8f00200 

// gic controller address
#define ZEDBOARD_GIC_BASE   0xf8f01000 

// no mpu in zynq 7000

#ifndef __io_address
#define __io_address(x) (x)
#endif

// IRQ 
#define PER_IRQ_BASE_NONE_SPI     32
#define PER_IRQ_BASE_SPI          16
#define ZEDBOARD_UART0_IRQ        27 // should plus 32 as this is none-spi
#define GLOBAL_TIMER_IRQ          11 // should plus 16 as this spi

// extern macro

// config the default value in defconf
#define SDRAM0_START UCONFIG_DRAM_START
#define SDRAM0_SIZE  UCONFIG_DRAM_SIZE

#define IO_SPACE_START PANDABOARD_IO_START
#define IO_SPACE_SIZE  0x1000000

//#define HAS_RAMDISK
//#define HAS_FRAMEBUFFER
//#define HAS_SHARED_KERNMEM
//#define KERNEL_PHY_BASE 0x100000

#ifndef __ASSEMBLY__

#define UART0_TX 		((volatile unsigned char*) PANDABOARD_UART0 + 0x00)
//#define INITIAL_LOAD    ((volatile uintptr_t *) (0x1000))

extern void board_init_early(void);
extern void board_init(void);

#endif

#endif
