#include <elf.h>
#include <mod.h>
#include <assert.h>

int apply_relocate(struct secthdr *sechdrs,
		   const char *strtab,
		   unsigned int symindex,
		   unsigned int relsec, struct module *mod)
{
  panic("apply_relocate not implemented for MIPS.");
	return -1;
}

/* Apply the given add relocation to the (simplified) ELF.  Return
	-error or 0 */
int apply_relocate_add(struct secthdr *sechdrs,
		       const char *strtab,
		       unsigned int symindex,
		       unsigned int relsec, struct module *mod)
{
  panic("apply_relocate not implemented for MIPS.");
	return -1;
}
