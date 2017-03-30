/*
 * =====================================================================================
 *
 *       Filename:  board.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/24/2012 01:21:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <board.h>
#include <picirq.h>
#include <serial.h>
#include <pmm.h>
#include <clock.h>

static const char *message = "Initializing ZEDBOARD...\n";

/*
static void put_string(const char *str)
{
	while (*str)
		serial_putc(*str++);
}
*/

#define EXT_CLK 38400000

void board_init_early()
{
	// put_string(message);
	gpio_init();
	gpio_test();

	serial_init(1, PER_IRQ_BASE_NONE_SPI + ZEDBOARD_UART1_IRQ);
	// Tianyu: we place pic init here
	pic_init2(ZEDBOARD_APU_BASE);
	kprintf("Tianyu: picirq inited! \n");
	// Tianyu: we place clock init here
	clock_init_arm(ZEDBOARD_TIMER0_BASE, GLOBAL_TIMER0_IRQ + PER_IRQ_BASE_SPI);
	kprintf("Tianyu: clock inited! \n");
}

void board_init()
{
    kprintf("zedboard init finished\n");
    serial_init_mmu();
	return;
}

/* no nand */
int check_nandflash()
{
	return 0;
}

struct nand_chip *get_nand_chip()
{
	return NULL;
}
