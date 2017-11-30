#include <types.h>
#include <kio.h>
#include <arm.h>
#include <error.h>

/* armv7m fixed base addresses */
const static uint8_t *V7M_SCS_BASE = (uint8_t*)0xE000E000;
//#define V7M_SCS_BASE		0xE000E000
#define V7M_NVIC_BASE		(V7M_SCS_BASE + 0x0100)
#define V7M_SCB_BASE		(V7M_SCS_BASE + 0x0D00)
#define V7M_PROC_FTR_BASE	(V7M_SCS_BASE + 0x0D78)
#define V7M_MPU_BASE		(V7M_SCS_BASE + 0x0D90)
#define V7M_FPU_BASE		(V7M_SCS_BASE + 0x0F30)
#define V7M_CACHE_MAINT_BASE	(V7M_SCS_BASE + 0x0F50)
#define V7M_ACCESS_CNTL_BASE	(V7M_SCS_BASE + 0x0F90)

#define V7M_SCB_VTOR		0x08

#define V7M_CACHE_REG_ICIALLU		((u32 *)(V7M_CACHE_MAINT_BASE + 0x00))
#define INVAL_ICACHE_POU		0
#define V7M_CACHE_REG_ICIMVALU		((u32 *)(V7M_CACHE_MAINT_BASE + 0x08))
#define V7M_CACHE_REG_DCIMVAC		((u32 *)(V7M_CACHE_MAINT_BASE + 0x0C))
#define V7M_CACHE_REG_DCISW		((u32 *)(V7M_CACHE_MAINT_BASE + 0x10))
#define V7M_CACHE_REG_DCCMVAU		((u32 *)(V7M_CACHE_MAINT_BASE + 0x14))
#define V7M_CACHE_REG_DCCMVAC		((u32 *)(V7M_CACHE_MAINT_BASE + 0x18))
#define V7M_CACHE_REG_DCCSW		((u32 *)(V7M_CACHE_MAINT_BASE + 0x1C))
#define V7M_CACHE_REG_DCCIMVAC		((u32 *)(V7M_CACHE_MAINT_BASE + 0x20))
#define V7M_CACHE_REG_DCCISW		((u32 *)(V7M_CACHE_MAINT_BASE + 0x24))

#define V7M_PROC_REG_CLIDR		((u32 *)(V7M_PROC_FTR_BASE + 0x00))
#define V7M_PROC_REG_CTR		((u32 *)(V7M_PROC_FTR_BASE + 0x04))
#define V7M_PROC_REG_CCSIDR		((u32 *)(V7M_PROC_FTR_BASE + 0x08))
#define MASK_NUM_WAYS			GENMASK(12, 3)
#define MASK_NUM_SETS			GENMASK(27, 13)
#define CLINE_SIZE_MASK			GENMASK(2, 0)
#define NUM_WAYS_SHIFT			3
#define NUM_SETS_SHIFT			13
#define V7M_PROC_REG_CSSELR		((u32 *)(V7M_PROC_FTR_BASE + 0x0C))
#define SEL_I_OR_D			BIT(0)

#define BITS_PER_LONG 32
#define BIT(nr)			(1UL << (nr))
#define GENMASK(h, l) \
	(((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#define WAYS_SHIFT			30
#define SETS_SHIFT			5

#define ARCH_DMA_MINALIGN 4096
#define FPGA_SUCCESS		0
#define FPGA_FAIL	-1
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

typedef uint32_t u32;
typedef uint32_t __u32;
typedef uint8_t u8;

#define writel(data, addr) outw((uintptr_t)addr, data)
#define readl(addr) inw((uintptr_t)addr)

#define debug(...) kprintf(__VA_ARGS__)
#define printf(...) kprintf(__VA_ARGS__)

#define roundup(x, y) (					\
{							\
	const typeof(y) __y = y;			\
	(((x) + (__y - 1)) / __y) * __y;		\
}							\
)

#define rounddown(x, y) (				\
{							\
	typeof(x) __x = (x);				\
	__x - (__x % (y));				\
}							\
)

#define ISB	asm volatile ("isb sy" : : : "memory")
#define DSB	asm volatile ("dsb sy" : : : "memory")
#define DMB	asm volatile ("dmb sy" : : : "memory")

#define isb()	ISB
#define dsb()	DSB
#define dmb()	DMB

#define ZYNQ_SYS_CTRL_BASEADDR		0xF8000000
#define ZYNQ_DEV_CFG_APB_BASEADDR	0xF8007000

#define __swab32(x) \
	((__u32)( \
		(((__u32)(x) & (__u32)0x000000ffUL) << 24) | \
		(((__u32)(x) & (__u32)0x0000ff00UL) <<  8) | \
		(((__u32)(x) & (__u32)0x00ff0000UL) >>  8) | \
		(((__u32)(x) & (__u32)0xff000000UL) >> 24) ))

struct slcr_regs {
	u32 scl; /* 0x0 */
	u32 slcr_lock; /* 0x4 */
	u32 slcr_unlock; /* 0x8 */
	u32 reserved0_1[61];
	u32 arm_pll_ctrl; /* 0x100 */
	u32 ddr_pll_ctrl; /* 0x104 */
	u32 io_pll_ctrl; /* 0x108 */
	u32 reserved0_2[5];
	u32 arm_clk_ctrl; /* 0x120 */
	u32 ddr_clk_ctrl; /* 0x124 */
	u32 dci_clk_ctrl; /* 0x128 */
	u32 aper_clk_ctrl; /* 0x12c */
	u32 reserved0_3[2];
	u32 gem0_rclk_ctrl; /* 0x138 */
	u32 gem1_rclk_ctrl; /* 0x13c */
	u32 gem0_clk_ctrl; /* 0x140 */
	u32 gem1_clk_ctrl; /* 0x144 */
	u32 smc_clk_ctrl; /* 0x148 */
	u32 lqspi_clk_ctrl; /* 0x14c */
	u32 sdio_clk_ctrl; /* 0x150 */
	u32 uart_clk_ctrl; /* 0x154 */
	u32 spi_clk_ctrl; /* 0x158 */
	u32 can_clk_ctrl; /* 0x15c */
	u32 can_mioclk_ctrl; /* 0x160 */
	u32 dbg_clk_ctrl; /* 0x164 */
	u32 pcap_clk_ctrl; /* 0x168 */
	u32 reserved0_4[1];
	u32 fpga0_clk_ctrl; /* 0x170 */
	u32 reserved0_5[3];
	u32 fpga1_clk_ctrl; /* 0x180 */
	u32 reserved0_6[3];
	u32 fpga2_clk_ctrl; /* 0x190 */
	u32 reserved0_7[3];
	u32 fpga3_clk_ctrl; /* 0x1a0 */
	u32 reserved0_8[8];
	u32 clk_621_true; /* 0x1c4 */
	u32 reserved1[14];
	u32 pss_rst_ctrl; /* 0x200 */
	u32 reserved2[15];
	u32 fpga_rst_ctrl; /* 0x240 */
	u32 reserved3[5];
	u32 reboot_status; /* 0x258 */
	u32 boot_mode; /* 0x25c */
	u32 reserved4[116];
	u32 trust_zone; /* 0x430 */ /* FIXME */
	u32 reserved5_1[63];
	u32 pss_idcode; /* 0x530 */
	u32 reserved5_2[51];
	u32 ddr_urgent; /* 0x600 */
	u32 reserved6[6];
	u32 ddr_urgent_sel; /* 0x61c */
	u32 reserved7[56];
	u32 mio_pin[54]; /* 0x700 - 0x7D4 */
	u32 reserved8[74];
	u32 lvl_shftr_en; /* 0x900 */
	u32 reserved9[3];
	u32 ocm_cfg; /* 0x910 */
};

extern struct slcr_regs *slcr_base;

static void clrbits_le32(volatile uint32_t* addr, uint32_t mask)
{
    (*addr) &= ~mask;
}

static void setbits_le32(volatile uint32_t* addr, uint32_t mask)
{
    (*addr) |= mask;
}

enum cache_type {
	DCACHE,
	ICACHE,
};

/* PoU : Point of Unification, Poc: Point of Coherency */
enum cache_action {
	INVALIDATE_POU,		/* i-cache invalidate by address */
	INVALIDATE_POC,		/* d-cache invalidate by address */
	INVALIDATE_SET_WAY,	/* d-cache invalidate by sets/ways */
	FLUSH_POU,		/* d-cache clean by address to the PoU */
	FLUSH_POC,		/* d-cache clean by address to the PoC */
	FLUSH_SET_WAY,		/* d-cache clean by sets/ways */
	FLUSH_INVAL_POC,	/* d-cache clean & invalidate by addr to PoC */
	FLUSH_INVAL_SET_WAY,	/* d-cache clean & invalidate by set/ways */
};

static u32 *get_action_reg_range(enum cache_action action)
{
	switch (action) {
	case INVALIDATE_POU:
		return V7M_CACHE_REG_ICIMVALU;
	case INVALIDATE_POC:
		return V7M_CACHE_REG_DCIMVAC;
	case FLUSH_POU:
		return V7M_CACHE_REG_DCCMVAU;
	case FLUSH_POC:
		return V7M_CACHE_REG_DCCMVAC;
	case FLUSH_INVAL_POC:
		return V7M_CACHE_REG_DCCIMVAC;
	default:
		break;
	}

	return NULL;
}

static u32 get_cline_size(enum cache_type type)
{
	u32 size;

	if (type == DCACHE)
		clrbits_le32(V7M_PROC_REG_CSSELR, BIT(SEL_I_OR_D));
	else if (type == ICACHE)
		setbits_le32(V7M_PROC_REG_CSSELR, BIT(SEL_I_OR_D));
	/* Make sure cache selection is effective for next memory access */
	dsb();

	size = readl(V7M_PROC_REG_CCSIDR) & CLINE_SIZE_MASK;
	/* Size enocoded as 2 less than log(no_of_words_in_cache_line) base 2 */
	size = 1 << (size + 2);
	debug("cache line size is %d\n", size);

	return size;
}

static int action_cache_range(enum cache_action action, u32 start_addr,
			      int64_t size)
{
	u32 cline_size;
	u32 *action_reg;
	enum cache_type type;

	action_reg = get_action_reg_range(action);
	if (!action_reg)
		return -E_INVAL;
	if (action == INVALIDATE_POU)
		type = ICACHE;
	else
		type = DCACHE;

	/* Cache line size is minium size for the cache action */
	cline_size = get_cline_size(type);
	/* Align start address to cache line boundary */
	start_addr &= ~(cline_size - 1);
	debug("total size for cache action = %llx\n", size);
	do {
		writel(start_addr, action_reg);
		size -= cline_size;
		start_addr += cline_size;
	} while (size > cline_size);

	/* Make sure cache action is effective for next memory access */
	dsb();
	isb();	/* Make sure instruction stream sees it */
	debug("cache action on range done\n");

	return 0;
}

static void flush_dcache_range(unsigned long start, unsigned long stop)
{
	if (action_cache_range(FLUSH_POC, start, stop - start)) {
		printf("ERR: D-cache not flushed\n");
		return;
	}
}
