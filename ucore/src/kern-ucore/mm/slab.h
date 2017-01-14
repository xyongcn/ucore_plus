#ifndef __SLAB_ALLOC
#define __SLAB_ALLOC
#include<spinlock.h>
#include<list.h>

#define CACHE_NAMELEN 20       //the length of the human readable name of cache, same as it is in linux 2.x
#define CFGS_OFF_SLAB 				0x00000001	//indicate the cache kept offslab
#define SLAB_HWCACHE_ALIGN 			0x00000010	//aligns the obj to L1 CPU cache
#define SLAB_MUST_HWCACHE_ALIGN 	0x00000020	//forces alignment
#define SLAB_NO_REAP				0x00000040	//never reap the slabs in this cache
#define SLAB_NO_GROW				0x00000080	
#define SLAB_KERNEL					0x00000100
#define SLAB_CTOR_CONSTRUCTOR 		0x00001000
#define DFLGS_GROWN					0x00010000  //TODO maybe replicated?

typedef uint32_t kmem_bufctl_t;

typedef struct kmem_cache_s kmem_cache_t;

struct kmem_cache_s{
    list_entry_t slabs_full;
    list_entry_t slabs_partial;
    list_entry_t slabs_free;
    size_t objsize;         //The size of each object in the slab
    size_t flags;           //determines how the allocator behaves
    size_t num;             //num of objects contained i each slab
    spinlock_s spinlock;

    //&gfp parameters
    uint32_t gfporder;      //indicates the 2^order pages size of the slab
    uint32_t gfpflags;      //flags for calling buddy system

    //color scheme
    size_t colour;	    //number of different offset can use
    uint32_t colour_off;	    //multiple to offset each object in the slab
    uint32_t colour_next;   //the next colour line to use, wrap back to 0 when reach colour
    uint32_t dflags;	    //flags modified during its lifetime							
	uint32_t growing;		//indicate it is growing to make it less likely to be eliminated

    kmem_cache_t *slabp_cache;  //if slab_desp is off_slab, it will be saved here;

    //constructor and destructor, may be NULL
    void(*ctor)(void*);

    char name[CACHE_NAMELEN];   //human readabl object name
    
    list_entry_t next;     //the next cache in the cache chain;
};

kmem_cache_t * kmem_cache_create(const char* name, size_t size, size_t offset, size_t flags, void (*ctor)(void*));

void *kmem_cache_alloc(kmem_cache_t *cachep, uint32_t flags);

void kmem_cache_free(kmem_cache_t* cachep, void *objp);

int kmem_cache_destroy(kmem_cache_t* cachep);

void* kmalloc(size_t size);

void kfree(void* objp);

void slab_init(void);

#endif
