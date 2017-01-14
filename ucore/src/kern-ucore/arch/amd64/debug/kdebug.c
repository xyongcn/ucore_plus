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

#define STACKFRAME_DEPTH 20

// To implent:
//   debuginfo_rip(debuginfo_eip)
// To check:
//   stab_binsearch()
struct ripdebuginfo {
	const char *rip_file;	// source code filename for rip
	int rip_line;		// source code line number for rip
	const char *rip_fn_name;	// name of function containing rip
	int rip_fn_namelen;	// length of function's name
	uintptr_t rip_fn_addr;	// start address of function
	int rip_fn_narg;	// number of function arguments
};

/* user STABS data structure  */
struct userstabdata {
	const struct stab *stabs;
	const struct stab *stab_end;
	const char *stabstr;
	const char *stabstr_end;
};

extern const struct stab __STAB_BEGIN__[];	// beginning of stabs table
extern const struct stab __STAB_END__[];	// end of stabs table
extern const char __STABSTR_BEGIN__[];	// beginning of string table
extern const char __STABSTR_END__[];	// end of string table

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
		//kprintf("\r\n");
		//print_debuginfo(rip - 1);
		rip = ((uint64_t *) rbp)[1];
		rbp = ((uint64_t *) rbp)[0];
	}
}

//Following are codes to be modified in order to enhance print_stackframe
void print_debuginfo(uintptr_t rip)
{
	struct ripdebuginfo info;
	if (debuginfo_rip(rip, &info) != 0) {
		kprintf("    <unknown>: -- 0x%08x --\n", rip);
	} else {
		char fnname[256];
		int j;
		for (j = 0; j < info.rip_fn_namelen; j++) {
			fnname[j] = info.rip_fn_name[j];
		}
		fnname[j] = '\0';
		kprintf("    %s:%d: %s+%d\r\n", info.rip_file, info.rip_line,
			fnname, rip - info.rip_fn_addr);
	}
}

/* *
 * stab_binsearch - according to the input, the initial value of
 * range [*@region_left, *@region_right], find a single stab entry
 * that includes the address @addr and matches the type @type,
 * and then save its boundary to the locations that pointed
 * by @region_left and @region_right.
 *
 * Some stab types are arranged in increasing order by instruction address.
 * For example, N_FUN stabs (stab entries with n_type == N_FUN), which
 * mark functions, and N_SO stabs, which mark source files.
 *
 * Given an instruction address, this function finds the single stab entry
 * of type @type that contains that address.
 *
 * The search takes place within the range [*@region_left, *@region_right].
 * Thus, to search an entire set of N stabs, you might do:
 *
 *      left = 0;
 *      right = N - 1;    (rightmost stab)
 *      stab_binsearch(stabs, &left, &right, type, addr);
 *
 * The search modifies *region_left and *region_right to bracket the @addr.
 * *@region_left points to the matching stab that contains @addr,
 * and *@region_right points just before the next stab.
 * If *@region_left > *region_right, then @addr is not contained in any
 * matching stab.
 *
 * For example, given these N_SO stabs:
 *      Index  Type   Address
 *      0      SO     f0100000
 *      13     SO     f0100040
 *      117    SO     f0100176
 *      118    SO     f0100178
 *      555    SO     f0100652
 *      556    SO     f0100654
 *      657    SO     f0100849
 * this code:
 *      left = 0, right = 657;
 *      stab_binsearch(stabs, &left, &right, N_SO, 0xf0100184);
 * will exit setting left = 118, right = 554.
 * */
static void
stab_binsearch(const struct stab *stabs, int *region_left, int *region_right,
	       int type, uintptr_t addr)
{
	int l = *region_left, r = *region_right, any_matches = 0;

	while (l <= r) {
		int true_m = (l + r) / 2, m = true_m;

		// search for earliest stab with right type
		while (m >= l && stabs[m].n_type != type) {
			m--;
		}
		if (m < l) {	// no match in [l, m]
			l = true_m + 1;
			continue;
		}
		// actual binary search
		any_matches = 1;
		if (stabs[m].n_value < addr) {
			*region_left = m;
			l = true_m + 1;
		} else if (stabs[m].n_value > addr) {
			*region_right = m - 1;
			r = m - 1;
		} else {
			// exact match for 'addr', but continue loop to find
			// *region_right
			*region_left = m;
			l = m;
			addr++;
		}
	}

	if (!any_matches) {
		*region_right = *region_left - 1;
	} else {
		// find rightmost region containing 'addr'
		l = *region_right;
		for (; l > *region_left && stabs[l].n_type != type; l--)
			/* do nothing */ ;
		*region_left = l;
	}
}

int debuginfo_rip(uintptr_t addr, struct ripdebuginfo *info)
{
	const struct stab *stabs, *stab_end;
	const char *stabstr, *stabstr_end;

	info->rip_file = "<unknown>";
	info->rip_line = 0;
	info->rip_fn_name = "<unknown>";
	info->rip_fn_namelen = 9;
	info->rip_fn_addr = addr;
	info->rip_fn_narg = 0;

	// find the relevant set of stabs (INCOMPLETE)
	if (addr >= KERNBASE) {
		stabs = __STAB_BEGIN__;
		stab_end = __STAB_END__;
		stabstr = __STABSTR_BEGIN__;
		stabstr_end = __STABSTR_END__;
	}
	else {
		// user-program linker script, tools/user.ld puts the information about the
		// program's stabs (included __STAB_BEGIN__, __STAB_END__, __STABSTR_BEGIN__,
		// and __STABSTR_END__) in a structure located at virtual address USTAB.
		const struct userstabdata *usd = (struct userstabdata *)USTAB;
		// make sure that debugger (current process) can access this memory
		struct mm_struct *mm;
		if (current == NULL || (mm = current->mm) == NULL) {
			return -1;
		}
		if (!user_mem_check
		    (mm, (uintptr_t) usd, sizeof(struct userstabdata), 0)) {
			return -1;
		}

		stabs = usd->stabs;
		stab_end = usd->stab_end;
		stabstr = usd->stabstr;
		stabstr_end = usd->stabstr_end;
		// make sure the STABS and string table memory is valid
		if (!user_mem_check
		    (mm, (uintptr_t) stabs,
		     (uintptr_t) stab_end - (uintptr_t) stabs, 0)) {
			return -1;
		}
		if (!user_mem_check
		    (mm, (uintptr_t) stabstr, stabstr_end - stabstr, 0)) {
			return -1;
		}
	}
	// String table validity checks
	if (stabstr_end <= stabstr || stabstr_end[-1] != 0) {
		return -1;
	}
	// Now we find the right stabs that define the function containing
	// 'rip'.  First, we find the basic source file containing 'rip'.
	// Then, we look in that source file for the function.  Then we look
	// for the line number.

	// Search the entire set of stabs for the source file (type N_SO).
	int lfile = 0, rfile = (stab_end - stabs) - 1;
	stab_binsearch(stabs, &lfile, &rfile, N_SO, addr);
	if (lfile == 0)
		return -1;

	// Search within that file's stabs for the function definition
	// (N_FUN).
	int lfun = lfile, rfun = rfile;
	int lline, rline;
	stab_binsearch(stabs, &lfun, &rfun, N_FUN, addr);

	if (lfun <= rfun) {
		// stabs[lfun] points to the function name
		// in the string table, but check bounds just in case.
		if (stabs[lfun].n_strx < stabstr_end - stabstr) {
			info->rip_fn_name = stabstr + stabs[lfun].n_strx;
		}
		info->rip_fn_addr = stabs[lfun].n_value;
		addr -= info->rip_fn_addr;
		// Search within the function definition for the line number.
		lline = lfun;
		rline = rfun;
	}
	else {
		// Couldn't find function stab!  Maybe we're in an assembly
		// file.  Search the whole file for the line number.
		info->rip_fn_addr = addr;
		lline = lfile;
		rline = rfile;
	}
	info->rip_fn_namelen =
	    strfind(info->rip_fn_name, ':') - info->rip_fn_name;

	// Search within [lline, rline] for the line number stab.
	// If found, set info->rip_line to the right line number.
	// If not found, return -1.
	stab_binsearch(stabs, &lline, &rline, N_SLINE, addr);
	if (lline <= rline) {
		info->rip_line = stabs[rline].n_desc;
	}
	else {
		return -1;
	}

	// Search backwards from the line number for the relevant filename stab.
	// We can't just use the "lfile" stab because inlined functions
	// can interpolate code from a different file!
	// Such included source files use the N_SOL stab type.
	while (lline >= lfile
	       && stabs[lline].n_type != N_SOL
	       && (stabs[lline].n_type != N_SO || !stabs[lline].n_value)) {
		lline--;
	}
	if (lline >= lfile && stabs[lline].n_strx < stabstr_end - stabstr) {
		info->rip_file = stabstr + stabs[lline].n_strx;
	}
	// Set rip_fn_narg to the number of arguments taken by the function,
	// or 0 if there was no containing function.
	if (lfun < rfun) {
		for (lline = lfun + 1;
		     lline < rfun && stabs[lline].n_type == N_PSYM; lline++) {
			info->rip_fn_narg++;
		}
	}
	return 0;
}

/* *
 * print_stackframe - print a list of the saved rip values from the nested 'call'
 * instructions that led to the current point of execution
 * */
void print_stackframe(void)
{
  print_stackframe_ext(read_rbp(), read_rip());
}
