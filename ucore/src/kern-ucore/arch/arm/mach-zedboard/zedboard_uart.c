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

#define ZYNQ_UART_MR_PARITY_NONE	0x00000020  /* No parity mode */

#define BAUDRATE_CONFIG 115200

static bool serial_exists = 0;

static uint32_t uart_base = ZEDBOARD_UART0;

struct uart_zynq {
	uint32_t control;			/* 0x00 - Control Register [8:0] */
	uint32_t mode;				/* 0x04 - Mode Register [10:0] */
	uint32_t reserved1[4];
	uint32_t baud_rate_gen;		/* 0x18 - Baud Rate Generator [15:0] */
	uint32_t reserved2[4];
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
			return (struct uart_zynq *) ZEDBOARD_UART1;
		default:
			// TODO a better default behavior
			return (struct uart_zynq *) ZEDBOARD_UART1;
	}
}

void serial_putc(int c);

static int serial_int_handler(int irq, void * data) {\
	extern void dev_stdin_write(char c);
	char c = cons_getc();
	serial_putc(c);
	dev_stdin_write(c);
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
int serial_init(const int port, uint32_t irq) {
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

	// register_irq(irq, serial_int_handler, NULL);
	// pic_enable(irq);

	return 0;
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

	while(! serial_test(port)) {
		return -1;
	}

	uint32_t ret = inw((uint32_t) & regs -> tx_rx_fifo);
	return ret;
}

void serial_clear() {
	return;
}

int serial_check() {
	return serial_exists;
}
