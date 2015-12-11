/**
	@Author: Tianyu Chen
	This is an attempt to get the UART support done. 
	For ZedBoard. 
*/

#include <types.h>
#include <arm.h>
#include <stdio.h>
#include <board.h>

/** **implement**
	*************
	serial_init()
	serial_puts()
	serial_putc()
	serial_getc()
*/

#define ZYNQ_UART_CR_TX_EN			0x00000010 /* TX enabled */
#define ZYNQ_UART_CR_RX_EN			0x00000004 /* RX enabled */
#define ZYNQ_UART_CR_TXRST			0x00000002 /* TX logic reset */
#define ZYNQ_UART_CR_RXRST			0x00000001 /* RX logic reset */

#define ZYNQ_UART_MR_PARITY_NONE	0x00000020  /* No parity mode */

#define BAUDRATE_CONFIG 115200

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

/*
 * This global array does NOT work.
 * Possibly because memory mapping is not complete at very early time.
 */
/*const struct uart_zynq * uart_zynq_ports[2] = {
	[0] = (struct uart_zynq *) ZEDBOARD_UART0,
	[1] = (struct uart_zynq *) ZEDBOARD_UART1,
};*/

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

/* setup baud rate */
static void serial_setbrg(const int port) {
	unsigned int calc_bauderror, bdiv, bgen;
	unsigned long calc_baud = 0;
	unsigned long baud;
	// setup a clock value
	// unsigned long clock = ;
	struct uart_zynq * regs = uart_zynq_ports(port);

	/*
	if(clock < 1000000 && BAUDRATE_CONFIG > 4800) {
		baud = 4800;
	} else {
		baud = BAUDRATE_CONFIG;
	}
	*/

	// calculate bdiv and bgen
	// assume that uart_ref_clk is 50 mhz
	bdiv = 6;
	bgen = 62;

	// write registers
	// writel(bdiv, & regs -> baud_rate_divider);
	// writel(bgen, & regs -> baud_rate_gen);
	outw((uint32_t) & regs -> baud_rate_divider, bdiv);
	outw((uint32_t) & regs -> baud_rate_gen, bgen);
}

/* init the serial port */
int serial_init(const int port) {
	struct uart_zynq * regs = uart_zynq_ports(port);

	if(! regs) {
		return -1;
	}

	// rx/tx enabe and reset
	// writel(ZYNQ_UART_CR_TX_EN | ZYNQ_UART_CR_RX_EN | ZYNQ_UART_CR_TXRST | ZYNQ_UART_CR_RXRST, & regs -> control);
	outw((uint32_t) & regs -> control, ZYNQ_UART_CR_TX_EN | ZYNQ_UART_CR_RX_EN | ZYNQ_UART_CR_TXRST | ZYNQ_UART_CR_RXRST);
	// no parity
	// writel(ZYNQ_UART_MR_PARITY_NONE, & regs -> mode);
	outw((uint32_t) & regs -> mode, ZYNQ_UART_MR_PARITY_NONE);

	serial_setbrg(port);

	return 0;
}

/* put char */
void serial_putc(int c) {
	const int port = 1;
	struct uart_zynq * regs = uart_zynq_ports(port);

	if(c == '\n') {
		// writel('\r', & regs -> tx_rx_fifo);
		outw((uint32_t) & regs -> tx_rx_fifo, '\r');
	}

	// writel(c, & regs -> tx_rx_fifo);
	outw((uint32_t) & regs -> tx_rx_fifo, c);
}

/* put string via serial port */
void serial_puts(const char * s) {
	while(* s) {
		serial_putc(* s ++);
	}
}

int serial_check() {
	return 0;
}

int serial_proc_data() {
	return 0;
}

void serial_clear() {

}
