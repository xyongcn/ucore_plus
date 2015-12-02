/*
 * zedboard_gpio.c
 *
 *  Created on: 2015-12-1
 *      Author: dstray
 */

#include <arm.h>
#include <board.h>

// ###
#define ZEDBOARD_GPIO 0xE000A000 //General Purpose Input / Output Base Address
#define ZEDBOARD_SLCR 0xF8000000 //System Level Control Registers Base Address
#define APER_CLK_CTRL 0x0000012C //AMBA Peripheral Clock Control Relative Address
// ###
#define GPIO_RST_CTRL 0x0000022C //GPIO Software Reset Control Relative Address
#define GPIO_CPU1X_RST 1 << 0
#define GPIO_CPU_1XCLKACT 1 << 22 //GPIO AMBA Clock control bits
//General Purpose I/O (gpio) Registers
#define DATA(x) 0x00000040 + 0x4 * x //Output Data
#define DATA_RO(x) 0x00000060 + 0x4 * x //Input Data
#define GPIO_BANK(x) 0x00000200 + 0x40 * x //GPIO Bank Base, x from 0 to 3
#define DIRM 0x00000004 //Direction mode
#define OEN 0x00000008 //Output enable

void gpio_resets()
{
	outw(ZEDBOARD_SLCR + GPIO_RST_CTRL, GPIO_CPU1X_RST & 0x0); //reset GPIO controller
	outw(ZEDBOARD_SLCR + APER_CLK_CTRL, GPIO_CPU_1XCLKACT & 0x1); //enable clock for GPIO
}

void gpio_pin_config()
{
	for (int i = 0; i != 4; i ++)
		outw(ZEDBOARD_GPIO + GPIO_BANK(i) + OEN, 0xFFFFFFFF);
	outw(ZEDBOARD_GPIO + GPIO_BANK(0) + DIRM, 0xFFFFFFFF);
	outw(ZEDBOARD_GPIO + GPIO_BANK(1) + DIRM, 0x0); //Configure MIO pins as intput
	outw(ZEDBOARD_GPIO + GPIO_BANK(2) + DIRM, 0xFFFFFFFF); //Configure EMIO pin as output
	outw(ZEDBOARD_GPIO + GPIO_BANK(3) + DIRM, 0xFFFFFFFF);
}

void gpio_init()
{
	gpio_resets();
	gpio_pin_config();
}

void gpio_write(uint32_t bank, uint32_t data)
{
	if (bank == 1)
		outw(ZEDBOARD_GPIO + DATA(1), 0x3FFFFF & data);
	else
		outw(ZEDBOARD_GPIO + DATA(bank), data);
}

uint32_t gpio_read(uint32_t bank)
{
	return inw(ZEDBOARD_GPIO + DATA_RO(bank));
}
