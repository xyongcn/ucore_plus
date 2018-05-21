#ifndef __KERN_TRAP_INTERRUPT_MANAGER_H__
#define __KERN_TRAP_INTERRUPT_MANAGER_H__

#include <types.h>

struct trapframe;

typedef bool (*interrupt_handler_t)(struct trapframe* trapframe);

void interrupt_manager_init();
void interrupt_manager_register_handler(uint32_t trapno, interrupt_handler_t handler);
bool interrupt_manager_process(struct trapframe* trapframe);

#endif
