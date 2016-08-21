#include <types.h>
#include <arch.h>
#include <stdio.h>
#include <string.h>
#include <memlayout.h>
#include <vmm.h>
#include <kdebug.h>
#include <kio.h>
#include <stab.h>
#include <proc.h>
#include <dwarf_line.h>

#define STACKFRAME_DEPTH 10

/* *
 * print_kerninfo - print the information about kernel, including the location
 * of kernel entry, the start addresses of data and text segements, the start
 * address of free memory and how many memory that kernel has used.
 * */
void print_kerninfo(void)
{
	extern char __kern_text_start[], __kern_ro_start[], __kern_data_start[];
	extern char edata[], end[], kern_init[];
	kprintf("Special kernel symbols:\n");
	kprintf("  __kern_text_start  %p\n", __kern_text_start);
	kprintf("  __kern_ro_start    %p\n", __kern_ro_start);
	kprintf("  __kern_data_start  %p\n", __kern_data_start);
	kprintf("  edata              %p\n", edata);
	kprintf("  end                %p\n", end);
	kprintf("  kern_init          %p\n", kern_init);
	kprintf("Kernel executable memory footprint: %dKB\n",
		(end - kern_init + 1023) / 1024);
}

static uint64_t read_rip(void) __attribute__ ((noinline));

static uint64_t read_rip(void)
{
	uint64_t rip;
	asm volatile ("movq 8(%%rbp), %0":"=r" (rip));
	return rip;
}

/*
 * print_stackframe_ext is a more flexible version for print_stackframe,
 * allowing the initial rbp and rip being specified.
 * NOTE: print_stackframe_ext is mostly used by giving rbp and rip in trapframe
 * so that it can track stackframe of a user-state program, if rbp-based stack
 * positioning is not optimized out (only true for gcc -O0 by default).
 * TODO：a more complete version of print_stackframe is in progress.
 */
void print_stackframe_ext(uint64_t rbp, uint64_t rip)
{
	int i, j;
	for (i = 0; rbp != 0 && rip != 0 && i < STACKFRAME_DEPTH; i++) {
		kprintf("rbp:%p rip:%p\n", rbp, rip);
    //kprintf("args: ");
		//uint64_t *args = (uint64_t *) rbp + 2;
		//for (j = 0; j < 4; j++) {
		//	kprintf("%p ", args[j]);
		//}
		rip = ((uint64_t *) rbp)[1];
		rbp = ((uint64_t *) rbp)[0];
	}
}

void print_stackframe(void)
{
 	void *ebp = read_rbp();
  void *eip = read_rip();

 	int i, j;
 	for (i = 0; ebp != 0 && i < STACKFRAME_DEPTH; i++) {
    struct dwarf_line_section_header *header = __DWARF_DEBUG_LINE_BEGIN__;
    struct dwarf_line_search_result search_result;
    dwarf_line_search_for_address(header, eip, &search_result);
    if(search_result.address != NULL &&
      (machine_word_t)eip - (machine_word_t)search_result.address < 0x100
    ) {
      kprintf("%p : %s Line %d\n", eip, search_result.file_name, search_result.line);
    }
    else {
      kprintf("%p : Unknown\n", eip);
    }
    eip = ((void**)ebp)[1];
    ebp = ((void**)ebp)[0];
 	}
}
