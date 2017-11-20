/**
	@Author: Tianyu Chen, Dstray Lee
	UART support for Zedboard
	Tsinghua Univ. 
*/

#include <types.h>
#include <arm.h>
#include <stdio.h>
#include <kio.h>
#include <string.h>
#include <sync.h>
#include <board.h>
#include <picirq.h>
#include <pmm.h>

/** **implement**
	*************
	serial_init()
	serial_puts()
	serial_putc()
	serial_getc()
*/

#define ZYNQ_UART_SR_TXFULL		0x00000010		/* TX FIFO full */
#define ZYNQ_UART_SR_RXEMPTY	0x00000002		/* RX FIFO empty */

#define ZYNQ_UART_CR_TX_EN			0x00000010 /* TX enabled */
#define ZYNQ_UART_CR_RX_EN			0x00000004 /* RX enabled */
#define ZYNQ_UART_CR_TXRST			0x00000002 /* TX logic reset */
#define ZYNQ_UART_CR_RXRST			0x00000001 /* RX logic reset */

#define ZYNQ_UART_TRIG_LEVEL_MASK   0x0000003F /* RxFIFO's' trigger level is 0-63 */
#define ZYNQ_UART_RX_TRIG_INT       (1)        /* Rx trig interrupt*/

#define ZYNQ_UART_MR_PARITY_NONE	0x00000020  /* No parity mode */

#define BAUDRATE_CONFIG 115200

static bool serial_exists = 0;

static uint32_t uart_base = ZEDBOARD_UART1;

struct uart_zynq {
	uint32_t control;			/* 0x00 - Control Register [8:0] */
	uint32_t mode;				/* 0x04 - Mode Register [10:0] */
    uint32_t intr_enable;       /* 0x08 - Interrupt enable register [12:0] read_only */
    uint32_t intr_disable;      /* 0x0C - Interrupt disable register [12:0] read_only */
    uint32_t intr_mask;         /* 0x10 - Interrupt mask [12:0] */
    uint32_t channel_intr_status;  /*0x14 - Interrupt status [12:0] */
	uint32_t baud_rate_gen;		/* 0x18 - Baud Rate Generator [15:0] */
    uint32_t placeholders1;
    uint32_t recv_fifo_trig_level; /* 0x20 - Receiver fifo trigger level [5:0] */
	uint32_t placeholders2[2];
	uint32_t channel_sts;		/* 0x2c - Channel Status [11:0] */
	uint32_t tx_rx_fifo;			/* 0x30 - FIFO [15:0] or [7:0] */
	uint32_t baud_rate_divider;	/* 0x34 - Baud Rate Divider [7:0] */
};

static inline struct uart_zynq * uart_zynq_ports(int port)
	__attribute__ ((always_inline));

static inline struct uart_zynq * uart_zynq_ports(int port)
{
	switch (port) {
		case 0:
			return (struct uart_zynq *) ZEDBOARD_UART0;
		case 1:
			//return (struct uart_zynq *) ZEDBOARD_UART1;
			return (struct uart_zynq *) uart_base;
		default:
			// TODO a better default behavior
			//return (struct uart_zynq *) ZEDBOARD_UART1;
			return (struct uart_zynq *) uart_base;
	}
}

void serial_putc(int c);

static int serial_int_handler(int irq, void * data) {
	extern void dev_stdin_write(char c);
	char c = cons_getc();
	//serial_putc(c);
	dev_stdin_write(c);
    
    // clear interrupt status
    const int port = 1;
    serial_clear(port);
	return 0;
}

/* setup baud rate */
static void serial_setbrg(const int port) {
	unsigned int calc_bauderror, bdiv, bgen;
	unsigned long calc_baud = 0;
	unsigned long baud;
	struct uart_zynq * regs = uart_zynq_ports(port);

	// calculate bdiv and bgen
	// assume that uart_ref_clk is 50 mhz
	bdiv = 6;
	bgen = 62;

	// write registers
	outw((uint32_t) & regs -> baud_rate_divider, bdiv);
	outw((uint32_t) & regs -> baud_rate_gen, bgen);
}

/* init the serial port */
int serial_init(const int port) {
	if(serial_exists) {
		return -1;
	}
	serial_exists = 1;

	struct uart_zynq * regs = uart_zynq_ports(port);

	if(! regs) {
		return -1;
	}

	// rx/tx enabe and reset
	outw((uint32_t) & regs -> control, ZYNQ_UART_CR_TX_EN | ZYNQ_UART_CR_RX_EN | ZYNQ_UART_CR_TXRST | ZYNQ_UART_CR_RXRST);
	// no parity
	outw((uint32_t) & regs -> mode, ZYNQ_UART_MR_PARITY_NONE);

	serial_setbrg(port);

	return 0;
}

int serial_init_remap_irq(uint32_t irq, int port) {
    
	void *newbase = __ucore_ioremap(uart_base, PGSIZE, 0);
	uart_base = (uint32_t) newbase;

    // enable rx interrupt and register it.
	struct uart_zynq * regs = uart_zynq_ports(port);
    while(serial_set_trigger_level(1, 1) != 1);  // set trigger level to 1
    while((inw((uint32_t) & regs -> intr_mask) & ZYNQ_UART_RX_TRIG_INT) != 1) {
        uint32_t en_reg = inw((uint32_t) & regs -> intr_enable);
        uint32_t dis_reg = inw((uint32_t) & regs -> intr_disable);
        en_reg |= (uint32_t)ZYNQ_UART_RX_TRIG_INT;
        dis_reg &= (~(uint32_t)ZYNQ_UART_RX_TRIG_INT);
        outw((uint32_t) & regs -> intr_enable, en_reg);
        outw((uint32_t) & regs -> intr_disable, dis_reg);
    }

	register_irq(irq, serial_int_handler, NULL);
	pic_enable(irq);  // useless, this func is not implemented
    kprintf("do ioremap on uart");
}

/* put char */
void serial_putc(int c) {
	const int port = 1;
	struct uart_zynq * regs = uart_zynq_ports(port);

	while((inw((uint32_t) & regs -> channel_sts) & ZYNQ_UART_SR_TXFULL) != 0) { }

	if(c == '\n') {
		outw((uint32_t) & regs -> tx_rx_fifo, '\r');
		while((inw((uint32_t) & regs -> channel_sts) & ZYNQ_UART_SR_TXFULL) != 0) { }
	}

	outw((uint32_t) & regs -> tx_rx_fifo, c);
}

/* put string via serial port */
void serial_puts(const char * s) {
	while(* s) {
		serial_putc(* s ++);
	}
}

static int serial_test(const int port) {
	struct uart_zynq * regs = uart_zynq_ports(port);

	return (inw((uint32_t) & regs -> channel_sts) & ZYNQ_UART_SR_RXEMPTY) == 0;
}

int serial_proc_data(void) {
	const int port = 1;

	struct uart_zynq * regs = uart_zynq_ports(port);

	if (! serial_test(port)) {
		return -1;
	}

	uint32_t ret = inw((uint32_t) & regs -> tx_rx_fifo);
	return ret;
}

void serial_clear(int port) {
	struct uart_zynq * regs = uart_zynq_ports(port);
    uint32_t intr_sts = inw((uint32_t) & regs -> channel_intr_status);
    intr_sts |= ZYNQ_UART_RX_TRIG_INT;
    outw((uint32_t) & regs -> channel_intr_status, intr_sts);
}

int serial_check() {
	return serial_exists;
}

/* 
 * zynq-7000 uart use a fifo for recv data, it has a trigger level
 * for more details, refer t ug585-zynq-7000-trm 19.3.5
 * */
int serial_set_trigger_level(int port, uint32_t level)
{
	struct uart_zynq * regs = uart_zynq_ports(port);
    outw((uint32_t) & regs -> recv_fifo_trig_level, level);
    return inw((uint32_t) & regs -> recv_fifo_trig_level) == (level & ZYNQ_UART_TRIG_LEVEL_MASK) ;
}



