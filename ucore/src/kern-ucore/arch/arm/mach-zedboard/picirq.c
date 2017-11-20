/**
	@Author: Tianyu Chen
	Tsinghua Univ. 2016
*/

#include <stdlib.h>
#include <types.h>
#include <arm.h>
#include <picirq.h>
#include <clock.h>
#include <serial.h>
#include <intr.h>
#include <kio.h>
#include <memlayout.h>
#include <assert.h>

#define ICCICR			0x00000100		// CPU Interface Control Register (RW)
#define ICCPMR			0x00000104		// CPU Interface Priority Mask Register (RW)
#define ICCIAR			0x0000010C		// Interrupt Acknowledge Register (RW)
#define ICCEOIR			0x00000110		// End of Interrupt Register (RW)

#define ICDDCR			0x00001000		// Distributor Control Register (RW)
#define ICDISER0		0x00001100		// Distributor Interrupt Set Enable Register 0 (RW)
#define ICDICER0		0x00001180		// Distributor Interrupt Clear Enable Register 0 (RW)
#define ICDISPR0		0x00001200		// Distributor Set Pending Register 0 (RW)
#define ICDIPR0			0x00001400		// Distributor Interrupt Priority Register 0 (RW)
#define ICDIPTR0		0x00001800		// Interrupt Processor Targets Register (RO)
#define ICDICFR0		0x00001C00		// Interrupt Configuration Register (RO)
#define ICDSGIR			0x00001F00		// Distributor Software Generated Interrupt Register (RW)

#define MAX_IRQS_NR  128

static uint32_t apu_base = 0;

// Current IRQ mask
// static uint32_t irq_mask = 0xFFFFFFFF & ~(1 << INT_UART0);
static bool did_init = 0;

struct irq_action {
	ucore_irq_handler_t handler;
	void *opaque;
};

struct irq_action actions[MAX_IRQS_NR];

void pic_disable(unsigned int irq) {
	// not implemented
}

void pic_enable(unsigned int irq) {
	if (irq >= MAX_IRQS_NR)
		return;
	int off = irq / 32;
	int j = irq % 32;

	outw(apu_base + ICCICR, 0);		// Disable CPU Interface (it is already disabled, but nevermind)
	outw(apu_base + ICDDCR, 0);		// Disable Distributor (it is already disabled, but nevermind)

	outw(apu_base + ICDISER0 + off * 4, 1 << j);
	outw(apu_base + ICDIPTR0 + (irq & ~0x3), 0x01010101);

	outw(apu_base + ICCICR, 3);		// Enable CPU Interface
	outw(apu_base + ICDDCR, 1);		// Enable Distributor
}

void register_irq(int irq, ucore_irq_handler_t handler, void * opaque) {
	if (irq >= MAX_IRQS_NR) {
		kprintf("WARNING: register_irq: irq>=%d\n", MAX_IRQS_NR);
		return;
	}
	actions[irq].handler = handler;
	actions[irq].opaque = opaque;
}

void pic_init_early() {
	// not implemented
}

void pic_init() {
	// not implemented
}

/* pic_init
 * initialize the interrupt, but doesn't enable them */
void pic_init2(uint32_t base) {
	if (did_init)
		return;

	did_init = 1;
	//disable all
	apu_base = base;
	outw(apu_base + ICCICR, 0);			// Disable CPU Interface (it is already disabled, but nevermind)
	outw(apu_base + ICDDCR, 0);			// Disable Distributor (it is already disabled, but nevermind)
	outw(apu_base + ICCPMR, 0xffff);	// All interrupts whose priority is > 0xff are unmasked

	// fill all 1
	int i;
	for(i = 0; i < 32; i ++) {
		outw(apu_base + ICDICER0 + (i << 2), ~0);
	}

	outw(apu_base + ICCICR, 3);			// Enable CPU Interface
	outw(apu_base + ICDDCR, 1);			// Enable Distributor

}

void irq_handler() {
	uint32_t intnr = inw(apu_base + ICCIAR) & 0x3FF;
	if(actions[intnr].handler) {
		(* actions[intnr].handler) (intnr, actions[intnr].opaque);
	} else {
		panic("Unhandled HW IRQ %d\n", intnr);
	}

	// end of interrupt
	outw(apu_base + ICCEOIR, intnr & 0x3FF);
}

/* irq_clear
 * 	Clear a pending interrupt request
 *  necessary when handling an irq */
void irq_clear(unsigned int source) {
	// should be done in irq handler for specific device
}
