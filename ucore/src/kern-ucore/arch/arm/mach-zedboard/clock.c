/**
	@Author: Tianyu Chen
	Tsinghua Univ. 2016
*/

#include <arm.h>
#include <board.h>
#include <clock.h>
#include <trap.h>
#include <stdio.h>
#include <kio.h>
#include <picirq.h>

/* global timer register addr */
#define GLOBAL_TIMER_OFFSET 0x200
#define GLOBAL_TIMER_COUNT0 0x00
#define GLOBAL_TIMER_COUNT1 0x04
#define GLOBAL_TIMER_CTL 0x08
#define GLOBAL_TIMER_STS 0x0c
#define GLOBAL_TIMER_CMP0 0x10
#define GLOBAL_TIMER_CMP1 0x14

/* timer interrupt @100Hz(10ms) */
/* global timer is aloways work on 1/2 CPU freqency (refer to ug585-zynq-7000-trm) */
/* default global timer clock freq is 400MHz */
#define GLOBAL_TIMER_DIV (256-1)
#define GLOBAL_TIMER_DIV_MASK 0xFFFF00FF
#define GLOBAL_TIMER_COMP (1562500-1)
/* enable irq, timer, comp, disable auto-increment */
#define GLOBAL_TIMER_CONFIG  0x7 
#define GLOBAL_TIMER_CONFIG_MASK 0xFFFFFFF0
/* just disable timer */
#define GLOBAL_TIMER_DIS 0xFFFFFFFE

static uint32_t timer_base = 0;

static void timer_clear(void)
{
    // reload count reg to 0
    // disable timer
    uint32_t reg_ctl = inw((uint32_t) (timer_base + GLOBAL_TIMER_CTL));
    reg_ctl &= GLOBAL_TIMER_DIS;
    outw((uint32_t) (timer_base + GLOBAL_TIMER_CTL), reg_ctl);

    // reload count
    outw((uint32_t) (timer_base + GLOBAL_TIMER_COUNT0), 0);
    outw((uint32_t) (timer_base + GLOBAL_TIMER_COUNT1), 0);

    // enable timer
    reg_ctl = inw((uint32_t) (timer_base + GLOBAL_TIMER_CTL));
    reg_ctl &= GLOBAL_TIMER_CONFIG_MASK;
    reg_ctl |= GLOBAL_TIMER_CONFIG;
    outw((uint32_t) (timer_base + GLOBAL_TIMER_CTL), reg_ctl);

    // clear interrupt bit
    outw((uint32_t) (timer_base + GLOBAL_TIMER_STS), 1);
}

static int timer_int_handler(int irq, void *data)
{
	__common_timer_int_handler();
	timer_clear();
	return 0;
}

static int timer_init(uint32_t base, int irq)
{
	timer_base = base + GLOBAL_TIMER_OFFSET;
    
    // init global timer registers
    uint32_t reg_ctl = inw((uint32_t) (timer_base + GLOBAL_TIMER_CTL));
    reg_ctl &= GLOBAL_TIMER_DIV_MASK;
    reg_ctl |= GLOBAL_TIMER_DIV;
    reg_ctl &= GLOBAL_TIMER_CONFIG_MASK;
    reg_ctl |= GLOBAL_TIMER_CONFIG;
    outw((uint32_t) (timer_base + GLOBAL_TIMER_CTL), reg_ctl);
    
    // set cmp regiser
    outw((uint32_t) (timer_base + GLOBAL_TIMER_CMP0), (uint32_t) GLOBAL_TIMER_COMP);

    register_irq(irq, timer_int_handler, 0);
	pic_enable(irq);
}

void clock_init_arm(uint32_t base, int irq) {
	// init zynq clock system
    // just use default clocking configurations

    // init global timer
    timer_init(base, irq);
}
