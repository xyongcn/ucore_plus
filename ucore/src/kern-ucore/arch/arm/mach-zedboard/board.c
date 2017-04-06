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
    const int port = 1;
	// put_string(message);
	gpio_init();
	gpio_test();

	serial_init(port);  // for the sake of debug, init serial in advance
}


void board_init()
{
    const int port = 1;
    // ioremap apu
    uint32_t apu_base = 
        (uint32_t) __ucore_ioremap(ZEDBOARD_APU_BASE, 3 * PGSIZE, 0);
    
    // enable interrupts(GIC)
	pic_init2(apu_base);

    // init timer and its interrupt
	clock_init_arm(apu_base, GLOBAL_TIMER0_IRQ + PER_IRQ_BASE_SPI);

    // do ioremap on uart and init its interrupt
    if (port == 1) 
        serial_init_remap_irq(PER_IRQ_BASE_NONE_SPI + ZEDBOARD_UART1_IRQ, port);
    else
        serial_init_remap_irq(PER_IRQ_BASE_NONE_SPI + ZEDBOARD_UART0_IRQ, port);

    kprintf("zedboard init finished\n");
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
