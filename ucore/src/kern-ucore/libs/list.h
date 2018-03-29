#ifndef __LIBS_LIST_H__
#define __LIBS_LIST_H__

#ifndef __ASSEMBLER__

#include <types.h>
#include <stddef.h>
/* *
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when manipulating
 * whole lists rather than single entries, as sometimes we already know
 * the next/prev entries and we can generate better code by using them
 * directly rather than using the generic single-entry routines.
 * */

struct list_entry {
	struct list_entry *prev, *next;
};

typedef struct list_entry list_entry_t;

static inline void list_init(list_entry_t * elm)
    __attribute__ ((always_inline));
static inline void list_add(list_entry_t * listelm, list_entry_t * elm)
    __attribute__ ((always_inline));
static inline void list_add_before(list_entry_t * listelm, list_entry_t * elm)
    __attribute__ ((always_inline));
static inline void list_add_after(list_entry_t * listelm, list_entry_t * elm)
    __attribute__ ((always_inline));
static inline void list_del(list_entry_t * listelm)
    __attribute__ ((always_inline));
static inline void list_del_init(list_entry_t * listelm)
    __attribute__ ((always_inline));
static inline bool list_empty(list_entry_t * list)
    __attribute__ ((always_inline));
static inline list_entry_t *list_next(list_entry_t * listelm)
    __attribute__ ((always_inline));
static inline list_entry_t *list_prev(list_entry_t * listelm)
    __attribute__ ((always_inline));

static inline void __list_add(list_entry_t * elm, list_entry_t * prev,
			      list_entry_t * next)
    __attribute__ ((always_inline));
static inline void __list_del(list_entry_t * prev, list_entry_t * next)
    __attribute__ ((always_inline));

/* *
 * list_init - initialize a new entry
 * @elm:        new entry to be initialized
 * */
static inline void list_init(list_entry_t * elm)
{
	elm->prev = elm->next = elm;
}

/* *
 * list_add - add a new entry
 * @listelm:    list head to add after
 * @elm:        new entry to be added
 *
 * Insert the new element @elm *after* the element @listelm which
 * is already in the list.
 * */
static inline void list_add(list_entry_t * listelm, list_entry_t * elm)
{
	list_add_after(listelm, elm);
}

/* *
 * list_add_before - add a new entry
 * @listelm:    list head to add before
 * @elm:        new entry to be added
 *
 * Insert the new element @elm *before* the element @listelm which
 * is already in the list.
 * */
static inline void list_add_before(list_entry_t * listelm, list_entry_t * elm)
{
	__list_add(elm, listelm->prev, listelm);
}

/* *
 * list_add_after - add a new entry
 * @listelm:    list head to add after
 * @elm:        new entry to be added
 *
 * Insert the new element @elm *after* the element @listelm which
 * is already in the list.
 * */
static inline void list_add_after(list_entry_t * listelm, list_entry_t * elm)
{
	__list_add(elm, listelm, listelm->next);
}

/* *
 * list_del - deletes entry from list
 * @listelm:    the element to delete from the list
 *
 * Note: list_empty() on @listelm does not return true after this, the entry is
 * in an undefined state.
 * */
static inline void list_del(list_entry_t * listelm)
{
	__list_del(listelm->prev, listelm->next);
}

/* *
 * list_del_init - deletes entry from list and reinitialize it.
 * @listelm:    the element to delete from the list.
 *
 * Note: list_empty() on @listelm returns true after this.
 * */
static inline void list_del_init(list_entry_t * listelm)
{
	list_del(listelm);
	list_init(listelm);
}

/* *
 * list_empty - tests whether a list is empty
 * @list:       the list to test.
 * */
static inline bool list_empty(list_entry_t * list)
{
	return list->next == list;
}

/* *
 * list_next - get the next entry
 * @listelm:    the list head
 **/
static list_entry_t *list_next(list_entry_t * listelm)
{
	return listelm->next;
}

/* *
 * list_prev - get the previous entry
 * @listelm:    the list head
 **/
static list_entry_t *list_prev(list_entry_t * listelm)
{
	return listelm->prev;
}

/* *
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 * */
static inline void
__list_add(list_entry_t * elm, list_entry_t * prev, list_entry_t * next)
{
	prev->next = next->prev = elm;
	elm->next = next;
	elm->prev = prev;
}

/* *
 * Delete a list entry by making the prev/next entries point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 * */
static inline void __list_del(list_entry_t * prev, list_entry_t * next)
{
	prev->next = next;
	next->prev = prev;
}

/**
 * list_str - get the struct for this entry
 * alias of list_entry in linux
 * @ptr: the &struct list_head pointer.
 * @type: the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 */
#define list_str(ptr, type, member) \
		container_of(ptr, type, member)



#endif /* !__ASSEMBLER__ */

#endif /* !__LIBS_LIST_H__ */
