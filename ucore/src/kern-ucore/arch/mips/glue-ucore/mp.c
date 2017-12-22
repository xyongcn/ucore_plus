#include "../../../../glue-kern/arch/mips/glue_mp.h"
#include <mp.h>
#include <proc.h>
#include <pmm.h>
#include <vmm.h>
#include <sysconf.h>

DEFINE_PERCPU_NOINIT(struct cpu, cpus);
void *percpu_offsets[NCPU];
PLS int pls_lapic_id;
PLS int pls_lcpu_idx;
PLS int pls_lcpu_count;
void mp_set_mm_pagetable(struct mm_struct *mm)
{
	if (mm != NULL && mm->pgdir != NULL)
		lcr3(PADDR(mm->pgdir));
	else
		lcr3(boot_cr3);
	tlb_invalidate_all();
}

void mp_tlb_invalidate(pgd_t * pgdir, uintptr_t la)
{
	tlb_invalidate_all();
}

void mp_tlb_update(pgd_t * pgdir, uintptr_t la)
{
	tlb_invalidate_all();
}

int mp_init(void)
{
  sysconf.lcpu_boot = 0;
  sysconf.lnuma_count = 0;
  sysconf.lcpu_count = 1;
  percpu_offsets[0] = __percpu_start;

	return 0;
}
