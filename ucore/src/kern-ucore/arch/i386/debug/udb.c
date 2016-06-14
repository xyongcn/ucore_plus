
#include <proc.h>
#include <udb.h>
#include <pmm.h>
#include <vmm.h>

enum DebugSignal {
    DEBUG_ATTACH = 0,
    DEBUG_WAIT = 1,
    DEBUG_CONTINUE = 2,
    DEBUG_STEPINTO = 3,
    DEBUG_STEPOVER = 4,
    DEBUG_SETBREAKPOINT = 5, 
    DEBUG_DELBREAKPOINT = 6, 
    DEBUG_PRINT = 7, 
    DEBUG_PRINT_REG = 8,
    DEBUG_BACKTRACE = 9,
};

struct DebugProcessInfo {
    int state;
    uint32_t pc;
};

/*
struct InstBreakpoint {
    uintptr_t addr;
    uintptr_t vaddr;
    uintptr_t content;
} instBp[MAXBREAKPOINT];
uint32_t instBpN = 0;
*/

struct udbBp_s {
    uintptr_t vaddr;
} udbBp[MAXBREAKPOINT];
uint32_t udbBpN = 0;

int nextStepCount = 0;
intptr_t lastPc;

void udbSleep() {
    bool intr_flag;
    local_intr_save(intr_flag);
    current->state = PROC_SLEEPING;
    current->wait_state = WT_CHILD;
    local_intr_restore(intr_flag);
    schedule();
}

uintptr_t* udbGetKaddr(struct proc_struct* proc, uintptr_t vaddr) {
    uintptr_t la = ROUNDDOWN(vaddr, PGSIZE);
    struct Page * page = get_page(proc->mm->pgdir, la, NULL);
    if(page == NULL)
        return 0;
    return (uintptr_t*)(page2kva(page) + (vaddr - la));
}

/*
int udbSetInstBp(struct proc_struct* proc, uintptr_t vaddr) {
    uintptr_t la = ROUNDDOWN(vaddr, PGSIZE);
    struct Page * page = get_page(proc->mm->pgdir, la, NULL);
    uint32_t* kaddr = page2kva(page) + (vaddr - la);
    instBp[instBpN].addr = kaddr;
    instBp[instBpN].vaddr = vaddr;
    instBp[instBpN++].content = *kaddr & 0xff;
    *kaddr = (*kaddr & 0xFFFFFF00) | 0xCC;
    return 0;
}

int udbFindInstBp(uintptr_t vaddr) {
    for(int i = 0; i < instBpN; ++i)
        if(instBp[i].addr == vaddr || instBp[i].vaddr == vaddr)
            return i;
    cprintf("[udb error] Instruction breakpoint not found!");
    return -1;
}

int udbUnsetInstBp(int no) {
    uintptr_t* addr = instBp[no].addr;
    *addr = (*addr & 0xFFFFFF00) | instBp[no].content;
    for(int i = no; i < instBpN; ++i)
        instBp[i] = instBp[i+1];
    instBpN --;
}
*/

int udbFindBp(uintptr_t vaddr) {
    for(int i = 0; i < udbBpN; ++i)
        if(udbBp[i].vaddr == vaddr)
            return i;
    return -1;
}

int udbAttach(char* argv[]) {
    int argc = 0;
    while(argv[argc])
        argc++;
    int ret = do_execve(argv[0], argc, argv);
    //udbSetBreakpoint(current, current->tf->tf_eip);
    current->tf->tf_eflags |= 0x100;
    nextStepCount = 1;
    return 0;
}

int udbWait(struct proc_struct* proc, struct DebugProcessInfo* pinfo) {
    struct proc_struct* childProc = proc;
    switch(childProc->state) {
    case PROC_SLEEPING:
        // do nothing
        break;
    case PROC_RUNNABLE:
        // sleep current process
        udbSleep();
        break;
    default:
        // child process has exited (unexpected?)
        return -1;
        break;
    }
    pinfo->state = (proc->state == PROC_ZOMBIE?-1:0);
    pinfo->pc = proc->tf->tf_eip;
    return 0;
}

int udbContinue(struct proc_struct* proc) {
    if(proc->state == PROC_ZOMBIE)
        return -1;
    wakeup_proc(proc);
    return 0;
}

int udbSetBp(struct proc_struct* proc, uintptr_t vaddr) {
    if(vaddr == 0)
        vaddr = lastPc;
    udbBp[udbBpN++].vaddr = vaddr;
    return vaddr;
}

int udbStepInto(struct proc_struct* proc) {
    nextStepCount = 1;
    return udbContinue(proc);
}

int udbStepOver(struct proc_struct* proc) {
    uintptr_t pc = current->tf->tf_eip;
    // uint32_t test = udbGetKaddr(proc, pc);
    return udbContinue(proc);
}

int udbPrint(struct proc_struct* proc, char* arg[]) {
    uintptr_t* vaddr = arg[0];
    int type = arg[2];
    uintptr_t* kaddr;
    if(type == 0)
        kaddr = udbGetKaddr(proc, vaddr);
    else {
        uintptr_t* uaddr = proc->tf->tf_regs.reg_ebp + (int)vaddr;
        kaddr = udbGetKaddr(proc, uaddr);
    }
    if(kaddr == 0) {
        snprintf(arg[1], 1024, "Cannot access corresponding memory");
        return -2;
    }
    else {
        snprintf(arg[1], 1024, "%d", *kaddr);
        return 0;
    }
    return 0;
}

char* regTab[9] = {
    "edi", 
    "esi", 
    "ebp",
    "oesp",
    "ebx", 
    "edx",
    "ecx",
    "eax",
    0
};

int udbPrintReg(struct proc_struct* proc, char* arg[]) {
    char* regStr = arg[0];
    for(int i = 0; regTab[i]; ++i) {
        if(strcmp(regTab[i], regStr) == 0) {
            kprintf("0x%08x\n", *((uint32_t*)(&(proc->tf->tf_regs)) + i));
            return 0;
        }
    }
    kprintf("No such register.\n");
    return -2;
}

int udbBacktrace(struct proc_struct* proc, char* arg[]) {
    char* buf = arg[0];
    uint32_t* ebp = proc->tf->tf_regs.reg_ebp;
    int i, j;
    for (i = 0; ebp != 0 && i < 10; i ++) {
        uint32_t* kaddr = udbGetKaddr(proc, ebp);
        uint32_t* eip = udbGetKaddr(proc, ebp + 1);
        snprintf(buf, 1024, "0x%08x 0x%08x ", ebp, *eip);
        buf += strlen(buf);
        /*
        uint32_t *args = (uint32_t *)ebp + 2;
        for (j = 0; j < 4; j ++) {
          cprintf("0x%08x ", args[j]);
        }
        cprintf("\n");
        */
        ebp = *(kaddr);
    }
    return 0;
}

int userDebug(uintptr_t pid, uint32_t sig, uint32_t arg) {
    struct proc_struct* proc = find_proc(pid);
    switch(sig) {
        case DEBUG_ATTACH:
            return udbAttach(arg);
        break;
        case DEBUG_WAIT:
            return udbWait(proc, arg);
        break;
        case DEBUG_SETBREAKPOINT:
            return udbSetBp(proc, arg);
        break;
        case DEBUG_CONTINUE:
            return udbContinue(proc);
        break;
        case DEBUG_STEPINTO:
            return udbStepInto(proc);
        break;
        case DEBUG_STEPOVER:
            return udbStepOver(proc);
        break;
        case DEBUG_PRINT:
            return udbPrint(proc, arg);
        break;
        case DEBUG_PRINT_REG:
            return udbPrintReg(proc, arg);
        break;
        case DEBUG_BACKTRACE:
            return udbBacktrace(proc, arg);
        break;
    }
}
/*
void udbOnTrap() {
    uintptr_t pc = current->tf->tf_eip;
    udbUnsetInstBp(udbFindBp(pc));
    struct proc_struct* parent = current->parent;
    switch(parent->state) {
    case PROC_SLEEPING:
        // wake up parent
        wakeup_proc(parent);
        break;
    case PROC_RUNNABLE:
        // cross fingers and wait parent's turn
        break;
    default:
        // udb doesnt even try to wait
        break;
    }
    udbSleep();
}
*/

void udbStepTrap() {
    uintptr_t pc = current->tf->tf_eip;
    lastPc = pc;
    //cprintf("0x%x\n", pc);
    nextStepCount = nextStepCount-1<0?-1:nextStepCount-1;
    if(nextStepCount == 0 || udbFindBp(pc) >= 0) {
        // cprintf("0x%x\n", pc);
        struct proc_struct* parent = current->parent;
        switch(parent->state) {
        case PROC_SLEEPING:
            // wake up parent
            wakeup_proc(parent);
            break;
        case PROC_RUNNABLE:
            // cross fingers and wait parent's turn
            break;
        default:
            // udb doesnt even try to wait
            break;
        }
        udbSleep();    
    }
}
