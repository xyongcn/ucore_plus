#ifndef __UDB_H_
#define __UDB_H_

#include <types.h>

int userDebug(uintptr_t pid, uint32_t sig, uint32_t arg) ;
void udbStepTrap();
#define MAXBREAKPOINT 10

#endif __UDB_H_