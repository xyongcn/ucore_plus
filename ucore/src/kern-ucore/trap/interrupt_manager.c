#include <list.h>
#include <slab.h>
#include <trap.h>
#include <stddef.h>
#include <assert.h>
#include "interrupt_manager.h"

#define TRAPNO_MIN 0x00
#define TRAPNO_MAX 0xFF

struct interrupt_handler_entry {
  interrupt_handler_t handler;
  list_entry_t list_entry;
};

static list_entry_t interrupt_handlers[TRAPNO_MAX];

void interrupt_manager_init()
{
  for(int i = 0; i < TRAPNO_MAX; i++) {
    list_init(&interrupt_handlers[i]);
  }
}

void interrupt_manager_register_handler(uint32_t trapno, interrupt_handler_t handler)
{
  assert(trapno >= TRAPNO_MIN && trapno <= TRAPNO_MAX);
  struct interrupt_handler_entry *handler_entry = kmalloc(sizeof(struct interrupt_handler_entry));
  handler_entry->handler = handler;
  list_entry_t *list = &interrupt_handlers[trapno];
  list_add(list, &handler_entry->list_entry);
}

bool interrupt_manager_process(struct trapframe* trapframe)
{
  uint32_t trapno = trapframe->tf_trapno;
  if(trapno < TRAPNO_MIN || trapno > TRAPNO_MAX) {
    return 0;
  }
  list_entry_t *list = &interrupt_handlers[trapno];
  for(list_entry_t* i = list_next(list); i != list; i = list_next(i)) {
    struct interrupt_handler_entry* entry = container_of(i, struct interrupt_handler_entry, list_entry);
    if(entry->handler(trapframe)) return 1;
  }
  return 0;
}
