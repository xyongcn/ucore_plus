/*
 * (C) Copyright 2012-2013, Xilinx, Michal Simek
 *
 * (C) Copyright 2012
 * Joe Hershberger <joe.hershberger@ni.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*#include <common.h>
#include <console.h>
#include <asm/io.h>
#include <fs.h>
#include <zynqpl.h>
#include <linux/sizes.h>
#include <asm/arch/hardware.h>
#include <asm/arch/sys_proto.h>*/

#include <pmm.h>
#include <zynq_compat.h>
#include <zynq_programmable_logic.h>

#define SZ_1M 0

#define ALIGN(x,a)		__ALIGN_MASK((x),(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))
//#include <zynq_programmable_logic.h>

#define get_timer(x) 0

struct devcfg_regs {
	u32 ctrl; /* 0x0 */
	u32 lock; /* 0x4 */
	u32 cfg; /* 0x8 */
	u32 int_sts; /* 0xc */
	u32 int_mask; /* 0x10 */
	u32 status; /* 0x14 */
	u32 dma_src_addr; /* 0x18 */
	u32 dma_dst_addr; /* 0x1c */
	u32 dma_src_len; /* 0x20 */
	u32 dma_dst_len; /* 0x24 */
	u32 rom_shadow; /* 0x28 */
	u32 reserved1[2];
	u32 unlock; /* 0x34 */
	u32 reserved2[18];
	u32 mctrl; /* 0x80 */
	u32 reserved3;
	u32 write_count; /* 0x88 */
	u32 read_count; /* 0x8c */
};

static struct devcfg_regs *devcfg_base = ((struct devcfg_regs *)ZYNQ_DEV_CFG_APB_BASEADDR);
static wait_queue_t __wait_queue, *wait_queue = &__wait_queue;

#define CONFIG_SYS_HZ 100000000

const static int DEVCFG_IRQ = 40;
#define DEVCFG_CTRL_PCFG_PROG_B		0x40000000
#define DEVCFG_ISR_FATAL_ERROR_MASK	0x00740040
#define DEVCFG_ISR_ERROR_FLAGS_MASK	0x00340840
#define DEVCFG_ISR_RX_FIFO_OV		0x00040000
#define DEVCFG_ISR_DMA_DONE		0x00002000
#define DEVCFG_ISR_PCFG_DONE		0x00000004
#define DEVCFG_STATUS_DMA_CMD_Q_F	0x80000000
#define DEVCFG_STATUS_DMA_CMD_Q_E	0x40000000
#define DEVCFG_STATUS_DMA_DONE_CNT_MASK	0x30000000
#define DEVCFG_STATUS_PCFG_INIT		0x00000010
#define DEVCFG_MCTRL_PCAP_LPBK		0x00000010
#define DEVCFG_MCTRL_RFIFO_FLUSH	0x00000002
#define DEVCFG_MCTRL_WFIFO_FLUSH	0x00000001

#ifndef CONFIG_SYS_FPGA_WAIT
#define CONFIG_SYS_FPGA_WAIT CONFIG_SYS_HZ/100	/* 10 ms */
#endif

#ifndef CONFIG_SYS_FPGA_PROG_TIME
#define CONFIG_SYS_FPGA_PROG_TIME	(CONFIG_SYS_HZ * 4) /* 4 s */
#endif

#define DUMMY_WORD	0xffffffff

/* Xilinx binary format header */
static const u32 bin_format[] = {
	DUMMY_WORD, /* Dummy words */
	DUMMY_WORD,
	DUMMY_WORD,
	DUMMY_WORD,
	DUMMY_WORD,
	DUMMY_WORD,
	DUMMY_WORD,
	DUMMY_WORD,
	0x000000bb, /* Sync word */
	0x11220044, /* Sync word */
	DUMMY_WORD,
	DUMMY_WORD,
	0xaa995566, /* Sync word */
};

#define SWAP_NO		1
#define SWAP_DONE	2

static uint32_t last_status = 0;

/*
 * Load the whole word from unaligned buffer
 * Keep in your mind that it is byte loading on little-endian system
 */
static u32 load_word(const void *buf, u32 swap)
{
	u32 word = 0;
	u8 *bitc = (u8 *)buf;
	int p;

	if (swap == SWAP_NO) {
		for (p = 0; p < 4; p++) {
			word <<= 8;
			word |= bitc[p];
		}
	} else {
		for (p = 3; p >= 0; p--) {
			word <<= 8;
			word |= bitc[p];
		}
	}

	return word;
}

static u32 check_header(const void *buf)
{
	u32 i, pattern;
	int swap = SWAP_NO;
	u32 *test = (u32 *)buf;

	debug("%s: Let's check bitstream header\n", __func__);

	/* Checking that passing bin is not a bitstream */
	for (i = 0; i < ARRAY_SIZE(bin_format); i++) {
		pattern = load_word(&test[i], swap);

		/*
		 * Bitstreams in binary format are swapped
		 * compare to regular bistream.
		 * Do not swap dummy word but if swap is done assume
		 * that parsing buffer is binary format
		 */
		if ((__swab32(pattern) != DUMMY_WORD) &&
		    (__swab32(pattern) == bin_format[i])) {
			pattern = __swab32(pattern);
			swap = SWAP_DONE;
			debug("%s: data swapped - let's swap\n", __func__);
		}

		debug("%s: %d/%x: pattern %x/%x bin_format\n", __func__, i,
		      (u32)&test[i], pattern, bin_format[i]);
		if (pattern != bin_format[i]) {
			debug("%s: Bitstream is not recognized\n", __func__);
			return 0;
		}
	}
	debug("%s: Found bitstream header at %x %s swapinng\n", __func__,
	      (u32)buf, swap == SWAP_NO ? "without" : "with");

	return swap;
}

static void *check_data(u8 *buf, size_t bsize, u32 *swap)
{
	kprintf("Entering check_data.\n");
	u32 word, p = 0; /* possition */

	/* Because buf doesn't need to be aligned let's read it by chars */
	for (p = 0; p < bsize; p++) {
		word = load_word(&buf[p], SWAP_NO);
		//debug("%s: word %x %x/%x\n", __func__, word, p, (u32)&buf[p]);

		/* Find the first bitstream dummy word */
		if (word == DUMMY_WORD) {
			//debug("%s: Found dummy word at position %x/%x\n",
			//      __func__, p, (u32)&buf[p]);
			*swap = check_header(&buf[p]);
			if (*swap) {
				kprintf("Leaving check_data %d.\n", __LINE__);
				/* FIXME add full bitstream checking here */
				return &buf[p];
			}
		}
		/* Loop can be huge - support CTRL + C */
		//if (ctrlc())
		//	return NULL;
	}
	kprintf("Leaving check_data %d.\n", __LINE__);
	return NULL;
}

static int zynq_dma_transfer(u32 srcbuf, u32 srclen, u32 dstbuf, u32 dstlen)
{
	unsigned long ts;
	u32 isr_status;

	/* Set up the transfer */
	writel(PADDR((u32)srcbuf), &devcfg_base->dma_src_addr);
	writel(dstbuf, &devcfg_base->dma_dst_addr);
	writel(srclen, &devcfg_base->dma_src_len);
	writel(dstlen, &devcfg_base->dma_dst_len);

	isr_status = readl(&devcfg_base->int_sts);

	/* Polling the PCAP_INIT status for Set */
	ts = get_timer(0);
	bool intr_flag;
	local_intr_save(intr_flag);
	wait_t __wait, *wait = &__wait;
	wait_current_set(wait_queue, wait, WT_KBD);
	local_intr_restore(intr_flag);
	schedule();

	isr_status = last_status;
	while (!(isr_status & DEVCFG_ISR_DMA_DONE)) {
		if (isr_status & DEVCFG_ISR_ERROR_FLAGS_MASK) {
			debug("%s: Error: isr = 0x%08X\n", __func__,
			      isr_status);
			debug("%s: Write count = 0x%08X\n", __func__,
			      readl(&devcfg_base->write_count));
			debug("%s: Read count = 0x%08X\n", __func__,
			      readl(&devcfg_base->read_count));

			return FPGA_FAIL;
		}
		if (get_timer(ts) > CONFIG_SYS_FPGA_PROG_TIME) {
			printf("%s: Timeout wait for DMA to complete\n",
			       __func__);
			return FPGA_FAIL;
		}
		//isr_status = readl(&devcfg_base->int_sts);
	}

	debug("%s: DMA transfer is done\n", __func__);

	/* Clear out the DMA status */
	writel(DEVCFG_ISR_DMA_DONE, &devcfg_base->int_sts);

	return FPGA_SUCCESS;
}

static int zynq_dma_xfer_init(bitstream_type bstype)
{
	u32 status, control, isr_status;
	unsigned long ts;

	/* Clear loopback bit */
	kprintf("DEBUG! %x %d\n", devcfg_base, __LINE__);
	clrbits_le32(&devcfg_base->mctrl, DEVCFG_MCTRL_PCAP_LPBK);
	kprintf("DEBUG! %x %d\n", devcfg_base, __LINE__);

	if (bstype != BIT_PARTIAL) {
		zynq_slcr_devcfg_disable();

		/* Setting PCFG_PROG_B signal to high */
		control = readl(&devcfg_base->ctrl);
		writel(control | DEVCFG_CTRL_PCFG_PROG_B, &devcfg_base->ctrl);
		/* Setting PCFG_PROG_B signal to low */
		writel(control & ~DEVCFG_CTRL_PCFG_PROG_B, &devcfg_base->ctrl);

		/* Polling the PCAP_INIT status for Reset */
		ts = get_timer(0);
		while (readl(&devcfg_base->status) & DEVCFG_STATUS_PCFG_INIT) {
			if (get_timer(ts) > CONFIG_SYS_FPGA_WAIT) {
				printf("%s: Timeout wait for INIT to clear\n",
				       __func__);
				return FPGA_FAIL;
			}
		}

		/* Setting PCFG_PROG_B signal to high */
		writel(control | DEVCFG_CTRL_PCFG_PROG_B, &devcfg_base->ctrl);

		/* Polling the PCAP_INIT status for Set */
		ts = get_timer(0);
		while (!(readl(&devcfg_base->status) &
			DEVCFG_STATUS_PCFG_INIT)) {
			if (get_timer(ts) > CONFIG_SYS_FPGA_WAIT) {
				printf("%s: Timeout wait for INIT to set\n",
				       __func__);
				return FPGA_FAIL;
			}
		}
	}

	isr_status = readl(&devcfg_base->int_sts);

	/* Clear it all, so if Boot ROM comes back, it can proceed */
	writel(0xFFFFFFFF, &devcfg_base->int_sts);

	if (isr_status & DEVCFG_ISR_FATAL_ERROR_MASK) {
		debug("%s: Fatal errors in PCAP 0x%X\n", __func__, isr_status);

		/* If RX FIFO overflow, need to flush RX FIFO first */
		if (isr_status & DEVCFG_ISR_RX_FIFO_OV) {
			writel(DEVCFG_MCTRL_RFIFO_FLUSH, &devcfg_base->mctrl);
			writel(0xFFFFFFFF, &devcfg_base->int_sts);
		}
		return FPGA_FAIL;
	}

	status = readl(&devcfg_base->status);

	debug("%s: Status = 0x%08X\n", __func__, status);

	if (status & DEVCFG_STATUS_DMA_CMD_Q_F) {
		debug("%s: Error: device busy\n", __func__);
		return FPGA_FAIL;
	}

	debug("%s: Device ready\n", __func__);

	if (!(status & DEVCFG_STATUS_DMA_CMD_Q_E)) {
		if (!(readl(&devcfg_base->int_sts) & DEVCFG_ISR_DMA_DONE)) {
			/* Error state, transfer cannot occur */
			debug("%s: ISR indicates error\n", __func__);
			return FPGA_FAIL;
		} else {
			/* Clear out the status */
			writel(DEVCFG_ISR_DMA_DONE, &devcfg_base->int_sts);
		}
	}

	if (status & DEVCFG_STATUS_DMA_DONE_CNT_MASK) {
		/* Clear the count of completed DMA transfers */
		writel(DEVCFG_STATUS_DMA_DONE_CNT_MASK, &devcfg_base->status);
	}

	return FPGA_SUCCESS;
}

static u32 *zynq_align_dma_buffer(u32 *buf, u32 len, u32 swap)
{
	kprintf("Entering %s %d\n", __func__, __LINE__);
	u32 *new_buf;
	u32 i;

	if ((u32)buf != ALIGN((u32)buf, ARCH_DMA_MINALIGN)) {
		new_buf = (u32 *)ALIGN((u32)buf, ARCH_DMA_MINALIGN);

		/*
		 * This might be dangerous but permits to flash if
		 * ARCH_DMA_MINALIGN is greater than header size
		 */
		if (new_buf > buf) {
			debug("%s: Aligned buffer is after buffer start\n",
			      __func__);
			new_buf -= ARCH_DMA_MINALIGN;
		}
		printf("%s: Align buffer at %x to %x(swap %d)\n", __func__,
		       (u32)buf, (u32)new_buf, swap);

		for (i = 0; i < (len/4); i++)
			new_buf[i] = load_word(&buf[i], swap);

		buf = new_buf;
	} else if (swap != SWAP_DONE) {
		/* For bitstream which are aligned */
		u32 *new_buf = (u32 *)buf;

		printf("%s: Bitstream is not swapped(%d) - swap it\n", __func__,
		       swap);

		for (i = 0; i < (len/4); i++)
			new_buf[i] = load_word(&buf[i], swap);
	}
	kprintf("Leaving %s %d\n", __func__, __LINE__);
	return buf;
}

static int zynq_validate_bitstream(xilinx_desc *desc, const void *buf,
				   size_t bsize, u32 blocksize, u32 *swap,
				   bitstream_type *bstype)
{
	u32 *buf_start;
	u32 diff;

	buf_start = check_data((u8 *)buf, blocksize, swap);

	if (!buf_start)
		return FPGA_FAIL;

	/* Check if data is postpone from start */
	diff = (u32)buf_start - (u32)buf;
	if (diff) {
		printf("%s: Bitstream is not validated yet (diff %x)\n",
		       __func__, diff);
		return FPGA_FAIL;
	}

	if ((u32)buf < SZ_1M) {
		printf("%s: Bitstream has to be placed up to 1MB (%x)\n",
		       __func__, (u32)buf);
		return FPGA_FAIL;
	}

	if (zynq_dma_xfer_init(*bstype))
		return FPGA_FAIL;

	return 0;
}

int zynq_load(xilinx_desc *desc, const void *buf, size_t bsize,
		     bitstream_type bstype)
{
	unsigned long ts; /* Timestamp */
	u32 isr_status, swap;

	/*
	 * send bsize inplace of blocksize as it was not a bitstream
	 * in chunks
	 */
	if (zynq_validate_bitstream(desc, buf, bsize, bsize, &swap,
				    &bstype))
		return FPGA_FAIL;

	buf = zynq_align_dma_buffer((u32 *)buf, bsize, swap);

	debug("%s: Source = 0x%08X\n", __func__, (u32)buf);
	debug("%s: Size = %zu\n", __func__, bsize);

	/* flush(clean & invalidate) d-cache range buf */
	flush_dcache_range((u32)buf, (u32)buf +
			   roundup(bsize, ARCH_DMA_MINALIGN));

	if (zynq_dma_transfer((u32)buf | 1, bsize >> 2, 0xffffffff, 0))
		return FPGA_FAIL;

	isr_status = readl(&devcfg_base->int_sts);
	/* Check FPGA configuration completion */
	ts = get_timer(0);
	while (!(isr_status & DEVCFG_ISR_PCFG_DONE)) {
		if (get_timer(ts) > CONFIG_SYS_FPGA_WAIT) {
			printf("%s: Timeout wait for FPGA to config\n",
			       __func__);
			return FPGA_FAIL;
		}
		isr_status = readl(&devcfg_base->int_sts);
	}

	debug("%s: FPGA config done\n", __func__);

	if (bstype != BIT_PARTIAL)
		zynq_slcr_devcfg_enable();

	return FPGA_SUCCESS;
}

static int zynq_programmable_logic_int_handler(int irq, void * data) {
	bool intr_flag;
	local_intr_save(intr_flag);
	uint32_t status = readl(&devcfg_base->int_sts);
	last_status = status;
	writel(status, &devcfg_base->int_sts);
	if (!wait_queue_empty(wait_queue)) {
		wakeup_queue(wait_queue, WT_KBD, 1);
	}
	local_intr_restore(intr_flag);
	return 0;
}


void zynq_programmable_logic_init()
{
	devcfg_base = __ucore_ioremap(ZYNQ_DEV_CFG_APB_BASEADDR, 8 * PGSIZE, 0);
	V7M_SCS_BASE = __ucore_ioremap(V7M_SCS_BASE, 8 * PGSIZE, 0);
	wait_queue_init(wait_queue);
	writel(~DEVCFG_ISR_DMA_DONE, &devcfg_base->int_mask);
	register_irq(DEVCFG_IRQ, zynq_programmable_logic_int_handler, NULL);
	slcr_base  = (struct slcr_regs *)__ucore_ioremap(ZYNQ_SYS_CTRL_BASEADDR, 8 * PGSIZE, 0);
	pic_enable(DEVCFG_IRQ);
}
