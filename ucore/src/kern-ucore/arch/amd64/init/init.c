#include <types.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <kdebug.h>
#include <picirq.h>
#include <trap.h>
#include <clock.h>
#include <intr.h>
#include <pmm.h>
#include <vmm.h>
#include <ide.h>
#include <fs.h>
#include <swap.h>
#include <proc.h>
#include <sched.h>
#include <kio.h>
#include <mp.h>
#include <mod.h>
#include <percpu.h>
#include <sysconf.h>
#include <lapic.h>
#include <multiboot.h>
#include <refcache.h>
#include <dde_kit/dde_kit.h>
#include <pci.h>
#include <network/e1000.h>

int kern_init(uint64_t, uint64_t) __attribute__ ((noreturn));

static void bootaps(void)
{
	int i;
	kprintf("starting to boot Application Processors!\n");
	for(i=0;i<sysconf.lcpu_count;i++){
		if(i == myid())
			continue;
		kprintf("booting cpu%d\n", i);
		cpu_up(i);
	}
}

struct e820map *e820map_addr = (struct e820map *)(0x8000 + PBASE);
static void mbmem2e820(Mbdata *mb)
{
	static struct e820map m;
	if(!(mb->flags & (1<<6)))
    		panic("multiboot header has no memory map");
	e820map_addr = &m;
  	uint8_t *p = (uint8_t*) VADDR_DIRECT(mb->mmap_addr);
  	uint8_t *ep = p + mb->mmap_length;
	int i = 0;
	while(p<ep){
		if(i>=E820MAX) break;
		struct Mbmem *mbmem = (Mbmem*)p;
		p += 4 + mbmem->size;
		m.map[i].addr = mbmem->base;
		m.map[i].size = mbmem->length;
		m.map[i].type = mbmem->type;
		i++;
	}
	m.nr_map = i;
}

char *initrd_begin, *initrd_end;
static void parse_initrd(Mbdata *mb)
{
	if(!mb->flags & (1<<3))
		kprintf("multiboot header has no modules\n");
	int i;
	struct Mbmod *mods = (struct Mbmod*)VADDR_DIRECT(mb->mods_addr);
	for(i=0;i<mb->mods_count;i++){
		kprintf("mbmod: %d %p %p\n", i,
		  mods[i].start, mods[i].end);
	}
	if(mb->mods_count<1)
		return;
	initrd_begin = VADDR_DIRECT(mods[0].start);
	initrd_end = VADDR_DIRECT(mods[0].end);
}

int kern_init(uint64_t mbmagic, uint64_t mbmem)
{
  enable_sse();
	extern char edata[], end[];
	memset(edata, 0, end - edata);

	/* percpu variable for CPU0 is preallocated */
	percpu_offsets[0] = __percpu_start;

	cons_init();		// init the console

	const char *message = "(THU.CST) os is loading ...";
	kprintf("%s\n\n", message);
	if(mbmagic == MULTIBOOT_BOOTLOADER_MAGIC){
		kprintf("Multiboot dectected: param %p\n", (void*)mbmem);
		mbmem2e820((Mbdata*)VADDR_DIRECT(mbmem));
		parse_initrd((Mbdata*)VADDR_DIRECT(mbmem));
	}

	print_kerninfo();

	/* get_cpu_var not available before tls_init() */
	hz_init();
	gdt_init(per_cpu_ptr(cpus, 0));
	tls_init(per_cpu_ptr(cpus, 0));
	acpitables_init();
	lapic_init();
	numa_init();

	pmm_init_numa();		// init physical memory management, numa awared
	/* map the lapic */
	lapic_init_late();

	//init the acpi stuff

	idt_init();		// init interrupt descriptor table
	pic_init();		// init interrupt controller

  syscall_init(); // init fast syscall using SYSCALL op

//	acpi_conf_init();


	percpu_init();
	cpus_init();
#ifdef UCONFIG_ENABLE_IPI
	ipi_init();
#endif

	refcache_init();

	vmm_init();		// init virtual memory management
	sched_init();		// init scheduler
	proc_init();		// init process table
	sync_init();		// init sync struct

	/* ext int */
	ioapic_init();
	acpi_init();

  pci_init(); //Init PCI Driver
  e1000_init();

	ide_init();		// init ide devices
#ifdef UCONFIG_SWAP
	swap_init();		// init swap
#endif
	fs_init();		// init fs

	clock_init();		// init clock interrupt
	mod_init();

	trap_init();

	//XXX put here?
	bootaps();

	intr_enable();		// enable irq interrupt
  extern struct e1000_driver* driver;
  char* test_str = "\x52\x54\x00\x12\x34\x56\xFF\xFF\xFF\xFF\xFF\xFF\x08\x00I have been waiting all this time\n"
  "for one to wait me, one to call mine\n"
  "So when you are near, all that you holds dear\n"
  "do you fear what you will find?\n";
  kprintf("Sending data?\n");
  int ret = e1000_send_packet(driver, test_str, 512);
  kprintf("ret = %d\n", ret);
  ret = e1000_send_packet(driver, test_str, 512);
  kprintf("ret = %d\n", ret);

#ifdef UCONFIG_HAVE_LINUX_DDE36_BASE
	dde_kit_init();
#endif

	/* do nothing */
	cpu_idle();		// run idle process
}

void do_halt(void)
{
	acpi_power_off();
	for (;;);
}
