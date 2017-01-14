//XXX WARNING: spinlock and interupt need carefully check here
#include <pmm.h>
#include <slab.h>
#include <types.h>
#include <kio.h>
#include <string.h>
#include <mmu.h>
#include <sem.h>
#include <cache.h>
#include <sync.h>

#define info 0

#define BYTES_PER_WORD sizeof(void*)
#define MAX_ORDER 11           //different with the def in buddy_pmm.c, consistent with linux4.8, TODO TEST: maybe result the same.
#define MAX_GFP_ORDER 5        //max size of pages 2^~ in a slab
#define SLAB_LIMIT      0xFFFFFFFEL	// the max value of obj number
#define BUFCTL_END 		0xFFFF		//end marker of a slabbuf_ctl

//TODO should be move away
//TODO add macro unlikely
//TODO PAGE_SLAB TODO TODO
//global vals, calculate when slab init
uint32_t offslab_limit;    
uint32_t slab_break_gfp_order;
semaphore_t cache_chain_sem;
static list_entry_t cache_chain;
/*
------------------------
Two main type of object in slab;
* kmem_cache_s: defines a cache desciptor to manage some slabs of a same object;
* slab_s: manage the objects in a slab;
------------------------
*/
typedef struct slab_s slab_t;
struct slab_s{
    list_entry_t    list;     //the linked list the slab belongs to, slab_full or slabs_partial;
    uint32_t 	    colouroff;//color offset from the base addr of the 1st obj in the slab
    void            *s_mem;   //the starting address of the first object within the slab.
    uint32_t        inuse;    //the number of active objects in the slab
    kmem_bufctl_t   free;     //the first free object in the slab.
};


typedef struct cache_sizes{
	size_t 		cs_size;
	kmem_cache_t   *cs_cachep;
	//kmem_cache_t   *cs_dmacachep
}cache_sizes_t;
//get the beginning of the kmem_bufctl_t array
#define slab_bufctl(slabp)					 \
	((kmem_bufctl_t *)(((slab_t*)slabp)+1))
// get the slab address according to the link element (see list.h)
#define le2cache(le, member)                 \
    to_struct((le), kmem_cache_t, member)
#define le2slab(le, member)					\
	to_struct((le), slab_t, member)			\
//set the info in page, which cache or page it belongs to 
#define SET_PAGE_CACHE(page, cachep)                                                \
    do {                                                                            \
        struct Page *__page = (struct Page *)(page);                                \
        kmem_cache_t **__cachepp = (kmem_cache_t **)&(__page->page_link.next);      \
        *__cachepp = (kmem_cache_t *)(cachep);                                      \
    } while (0)

#define SET_PAGE_SLAB(page, slabp)                                                  \
    do {                                                                            \
        struct Page *__page = (struct Page *)(page);                                \
        slab_t **__cachepp = (slab_t **)&(__page->page_link.prev);                  \
        *__cachepp = (slab_t *)(slabp);                                             \
    } while (0)

#define GET_PAGE_CACHE(page)                                \
    (kmem_cache_t *)((page)->page_link.next)

#define GET_PAGE_SLAB(page)                                 \
    (slab_t *)((page)->page_link.prev)

//the cache of the cache(really awesome, how could i forget this)
static kmem_cache_t cache_cache = {
objsize:        sizeof(kmem_cache_t),
flags:          SLAB_NO_REAP,
spinlock:       {0},
name:           "kmem_cache",
colour_off:		L1_CACHE_BYTES
};

static cache_sizes_t cache_sizes[] = {
	{    32, NULL},
	{    64, NULL},
	{   128, NULL},
	{   256, NULL},
	{   512, NULL},
	{  1024, NULL},
	{  2048, NULL},
	{  4096, NULL},
	{  8192, NULL},
	{ 16384, NULL},
	{ 32768, NULL},
	{ 65536, NULL},
	{131072, NULL},
	{     0, NULL}
};


static inline void kmem_cache_init_objs(kmem_cache_t *cachep, slab_t *slabp, uint32_t ctor_flags);
void *kmem_cache_alloc(kmem_cache_t *cachep, uint32_t flags);
void BUG(const char* bug_location, char* bug_info);

static inline void kmem_freepages(kmem_cache_t* cachep, void* addr){
	unsigned long i = (1<<cachep->gfporder);
	struct Page *page = kva2page(addr);

	while(i--){
		ClearPageSlab(page);
		page++;
	}
	free_pages(kva2page(addr), 1<<cachep->gfporder);	
}

/*   
-----------------------------------------------------------
			Section on CACHE
-----------------------------------------------------------
*/


/*kmem_cache_estimate--Determine how many obj will be put in a slab, and the space left unused.
 *@gfporder: indicate the order of the pages in a slab
 *@size: size of the object
 *@flags: flags of the cache
 *@left_over: the number of bytes left in the slab, meant to be returned.
 *@num: the number of obj can be put in the slab, meant to be returned.
 */
static void kmem_cache_estimate(uint32_t gfporder, size_t size,
        int flags, size_t *left_over, uint32_t *num)
{
	int i=0;

	size_t wastage = PGSIZE<<gfporder;
	size_t base = 0;
	size_t extra = 0;
	
	if(!(flags & CFGS_OFF_SLAB)){
		base = sizeof(slab_t);
		extra = sizeof(kmem_bufctl_t);
	}
	/*obj_num is the num of objs, solve the equation $i*size + base + i*extra <= wastage$*/
	//XXX I don't get why `while(i*size + L1_CACHE_ALIGN(base+i*extra)<=wastage)`
	//which will cause error stray, even I retype it again and again
	size_t used = i*size + L1_CACHE_ALIGN(base+i*extra);
	while(used <= wastage){
		i++;
		used = i*size + L1_CACHE_ALIGN(base + i*extra);
	}
	if(i > 0)
		i--;
	if(i > SLAB_LIMIT)
		i = SLAB_LIMIT;

	*num = i;
	wastage -= i*size;
	wastage -= L1_CACHE_ALIGN(base+i*extra);
	*left_over = wastage;
}

void kmem_cache_free(kmem_cache_t *cachep, void *objp);
kmem_cache_t *kmem_find_general_cachep(size_t size);
static void kmem_slab_destroy(kmem_cache_t* cachep, slab_t* slabp);
//Not defined in the original virsion, for caches are already organized as an array. 
/*kmem_cache_create--create a new cache, and check some para
 *@name: human readable name of a object(cache)
 *@size: the size of the object
 *@flags: cache flags
 *@ctor: constructor of the objects, can be left NULL
 */
kmem_cache_t * kmem_cache_create(const char* name, size_t size, 
        size_t offset, size_t flags,
        void (*ctor)(void*))
{
    const char* bug_location = "kmem_create: ";
    size_t left_over, align, slab_size;
    
    kmem_cache_t *cachep = NULL;
    
    /*check usage bugs
     */
    if((!name)||
       ((strlen(name) >= CACHE_NAMELEN -1)) ||
       //((in_interrupt()||
       (size < BYTES_PER_WORD) ||
       (size > (1<<MAX_ORDER)*PGSIZE) ||
       (offset < 0 || offset > size))
        BUG(bug_location,"bad parameter usage");
    
    cachep = (kmem_cache_t *)kmem_cache_alloc(&cache_cache, SLAB_KERNEL);
    //simply allocate a new cache 
    if(!cachep)
        goto opps;
    memset(cachep, 0, sizeof(kmem_cache_t));    //TODO shouldbe put in kmem_cache_alloc?
    
    //aligns the object ro the wordsize
    if(size & (BYTES_PER_WORD-1)){
        size += (BYTES_PER_WORD-1);
        size &= ~(BYTES_PER_WORD-1);
#if info
        kprintf("SLAB CACHE CREATE: Forcing Alignment--%s\n", name);
#endif
    }


    //determine the value of align
    align = BYTES_PER_WORD;
    //if requested aligns the obj to the L1 CPU cache
    if(flags & SLAB_HWCACHE_ALIGN)
	align = L1_CACHE_BYTES;
    //keep slab_t offslab when too large object
    if(size >= (PGSIZE>>3))
        flags |= CFGS_OFF_SLAB;
    //TODO Comments needed here
	if(flags & SLAB_HWCACHE_ALIGN){
		while (size < align / 2)
	    	align /= 2;
		size = (size+align-1)&(~(align-1));
    }

    //calculate num of objects in a slab and adjust the size of a slab
    do{
        uint32_t break_flag = 0;
cal_wastage:
        kmem_cache_estimate(cachep->gfporder, size, flags, &left_over, &cachep->num);
        //set if the num of objs fitting on the slab exceeds the num that can be kept in the slab when offslab descrp are used
        if(break_flag)
            break;
        //the order size of pages in a slab must not be exceeded
        if(cachep->gfporder >= MAX_GFP_ORDER)
            break;
        if(!cachep->num)
            goto next;
        //if offslab, but num of obj 
        if(flags & CFGS_OFF_SLAB && cachep->num > offslab_limit){
            cachep->gfporder--;
            break_flag++;
            goto cal_wastage;
        }
        if(cachep->gfporder >= slab_break_gfp_order)
            break;
        //make sure slab left_over is less than 1/8 of a slab
        if((left_over*8) <= (PGSIZE<<cachep->gfporder))
            break;
        //if internal fragment is too high, increase slab size and recalculate
next:
        cachep->gfporder++;
    }while(1);

    //after recalculate, if the object isstill not fit, error
    if(!cachep->num){
        kprintf("kmem_cache_create: could not create cache %s.\n", name);
        kmem_cache_free(&cache_cache, cachep);
        cachep = NULL;
        goto opps;
    }
    //calculate slab_size, align to L1 cache
    slab_size = L1_CACHE_ALIGN(cachep->num * sizeof(kmem_bufctl_t) + sizeof(slab_t));
    //if enough space left for slab descriptor, move it into slab and update flags and slab leftover
    if(flags & CFGS_OFF_SLAB && left_over >= slab_size){
        flags &= ~CFGS_OFF_SLAB;
        left_over -= slab_size;
    }
	
	//calculate colour offsets
	offset += (align-1);
	offset &= ~(align -1);
	if(!offset)
		offset = L1_CACHE_BYTES;
	cachep->colour_off = offset;
	cachep->colour = left_over/offset;

    cachep->flags = flags;
    cachep->gfpflags = 0;
    spinlock_init(&cachep->spinlock);
    cachep->objsize = size;

    list_init(&cachep->slabs_full);
    list_init(&cachep->slabs_partial);
	list_init(&cachep->slabs_free);
    //if off slab, slab will be put in a slab cache
    if(flags & CFGS_OFF_SLAB)
        cachep->slabp_cache = kmem_find_general_cachep(slab_size);

    cachep->ctor = ctor;
    strcpy(cachep->name, name);
    
    //loop the list to ensure no existed cache has the same name with it
    down(&cache_chain_sem);
    {
        list_entry_t *p;

		p = list_next(&cache_chain);
        while(p != &cache_chain){
            if(!strcmp(le2cache(p, next)->name,name))
                BUG(bug_location, "Cache cannot be created--same cache name");
        p = list_next(p);
		}
    }

    list_add(&cache_chain, &cachep->next);
    up(&cache_chain_sem);
opps:
    return cachep;
}
/*__kmem_cache_shrink_locked: the main function to free slabs;
 * &cachep: the cache to free from
 */
static int __kmem_cache_shrink_locked(kmem_cache_t *cachep, int flags)
{
	slab_t* slabp;
	int ret = 0;

	while(!cachep->growing){
		list_entry_t *p;
		//get the last slab in the free list
		p = cachep->slabs_free.prev;
		if(p == &cachep->slabs_free)
			break;

		slabp = le2slab(cachep->slabs_free.prev, list);
		list_del(&slabp->list);

		spin_unlock_irqrestore(&cachep->spinlock, flags);	
		kmem_slab_destroy(cachep, slabp);
		ret++;
		spin_lock_irqsave(&cachep->spinlock, flags);
	}
	return ret;
}
/*__kmem_cache_shrink: remove the slabs form free and return the number of pages 
 * shrinked
 * @cachep: the cache shrink with
 */
static int __kmem_cache_shrink(kmem_cache_t* cachep)
{
	int ret;
	int flags;
	spin_lock_irqsave(&cachep->spinlock, flags); 
    __kmem_cache_shrink_locked(cachep, flags);
	//checks full and partial are empty
	ret = (int)(!list_empty(&cachep->slabs_full) || !list_empty(&cachep->slabs_partial));
	spin_unlock_irqrestore(&cachep->spinlock, flags);
	return ret;
}

/*kmem_cache_destroy: destroy a cache
 * @cachep: the cache to be destroyed
 */
int kmem_cache_destroy(kmem_cache_t* cachep)
{
	static kmem_cache_t* cache_schp;

	if(!cachep || cachep->growing)
		BUG("Cache Destroy", "Unable to destroy the cache");
	
	down(&cache_chain_sem);
	if(cache_schp == cachep)
		cache_schp = le2cache(cachep->next.next, next);
	list_del(&cachep->next);
	up(&cache_chain_sem);

	//shrink the cache to free all the slabs
	if(__kmem_cache_shrink(cachep)){
		kprintf("kmem_cache_destroy: cannot free all the objects %p\n", cachep);
		down(&cache_chain_sem);
		list_add(&cache_chain, &cachep->next);
		up(&cache_chain_sem);
		return 0;
	}

	kmem_cache_free(&cache_cache, cachep);
	return 1;
}
/*-------------------------------------------------------------------------
 *			      Section on slab
 *------------------------------------------------------------------------
 */

//Function the same.
//The original codes need less input, so some address mapping is essential
/*kmem_cache_slabmgmt: either allocate space to keep the slab descriptor **off cache**
 * or reserve enough space at the beginning of the slab for the descriptor and bufctl **in the cache**
 * @cachep: the cache which slab allocated to
 * @objp: points to the **beginning of the slab**
 * @local_flags: flags for cache
 */
static inline slab_t *kmem_cache_slabmgmt(kmem_cache_t *cachep, void *objp,
		uint32_t colour_off, int local_flags)
{
	slab_t *slabp;
	void* startadd = page2kva(objp);
		
	if(cachep->flags & CFGS_OFF_SLAB){
#if info
		kprintf("in slab\n");
#endif
		slabp = kmem_cache_alloc(cachep->slabp_cache, local_flags);
		if(!slabp)
			return NULL;
	}
	else{
		slabp = startadd+colour_off;
		colour_off += L1_CACHE_ALIGN(cachep->num*sizeof(kmem_bufctl_t)
				+ sizeof(slab_t));		
	}
	slabp->inuse = 0;
	slabp->colouroff = colour_off;
	slabp->s_mem = startadd+colour_off;

	return slabp;
}
//Most important function in slab
/*kmem_cache_grow: allocate memory, link pages for the slab, initialize obj in the slab and add slab to the cache 
 *@cachep: the cache above the slab
 *@flags: flags for slab
 */
static int kmem_cache_grow(kmem_cache_t *cachep, int flags)
{
	int 		order;
	bool		save_flags;
	uint32_t 	ctor_flags;
	slab_t   	*slabp;
	size_t 		offset;

	if(flags & SLAB_NO_GROW)
		return 0;

	ctor_flags = SLAB_CTOR_CONSTRUCTOR;
	//aquire an int-safe lock for accessing the cache descriptor
	spin_lock_irqsave(&cachep->spinlock, save_flags);
	
	offset = cachep->colour_next;
	cachep->colour_next++;
	if(cachep->colour_next >= cachep->colour)
		cachep->colour_next = 0;
	offset *= cachep->colour_off;
	cachep->dflags |= DFLGS_GROWN;	//make the slab growing, avoiding to be reaped
	
	cachep->growing++;
	spin_unlock_irqrestore(&cachep->spinlock, save_flags);	

	struct Page* objp = alloc_pages(1<<cachep->gfporder);
	if(!objp)
		goto failed;
#if info
	kprintf("New page of slab: %p\n", page2kva(objp));
#endif
	if(!(slabp = kmem_cache_slabmgmt(cachep, objp, offset, flags)))
		goto opps1;
	//link the pages for the slab
	order = 1 << cachep->gfporder;
	do{
               //setup this page in the free list (see memlayout.h: struct page)???
               SET_PAGE_CACHE(objp, cachep);
               SET_PAGE_SLAB(objp, slabp);
               //this page is used for slab
               SetPageSlab(objp);
               objp = NEXT_PAGE(objp);
	}while(--order);

	kmem_cache_init_objs(cachep, slabp, ctor_flags);

	//initialize all objects
	spin_lock_irqsave(&cachep->spinlock, save_flags);
	cachep->growing--;		

	list_add_before(&cachep->slabs_free, &slabp->list);
	
	spin_unlock_irqrestore(&cachep->spinlock, save_flags);
	
	return 1;

opps1:
	kmem_freepages(cachep,  objp);
failed:
	spin_lock_irqsave(&cachep->spinlock, save_flags);
	cachep->growing--;
	spin_unlock_irqrestore(&cachep->spinlock, save_flags);
	return 0;
}

/*kmem_find_general_cachep: **off slab**, find the appropriate size cache to use and will be  
 *@size: the size of the slab descriptor
*/
kmem_cache_t *kmem_find_general_cachep(size_t size){
	cache_sizes_t *csizep = cache_sizes;

	for(; csizep->cs_size; csizep++){
		if(size > csizep->cs_size)
			continue;
		break;
	}
	return csizep->cs_cachep;
}

/*kmem_slab_destroy: destroy the slab
 * @cachep: the cache whose slabs will be destroyed
 * @slabp: the slab to be destroyed
 */
static void kmem_slab_destroy(kmem_cache_t* cachep, slab_t* slabp){
	kmem_freepages(cachep, slabp->s_mem-slabp->colouroff);
	if(cachep->flags&CFGS_OFF_SLAB)
		kmem_cache_free(cachep->slabp_cache, slabp);
	list_del(&slabp->list);
}
//----------------------------------------------
//					obj section
//----------------------------------------------

/*kmem_cache_init_objs: initiate all the objs in a slab
 *@cachep: the obj needed to init in
 *@slabp: same as above
 *@ctor_flags: flags needed to initiate 
 */
static inline void kmem_cache_init_objs(kmem_cache_t *cachep,
		slab_t *slabp, uint32_t ctor_flags)
{
	int i;
	for(i = 0;	i < cachep->num; i++){
	//count the addr of the obj
	void* obj = slabp->s_mem+cachep->objsize*i;
	//call the constructor if available
	if(cachep->ctor)
		cachep->ctor(obj);

	slab_bufctl(slabp)[i] = i+1;
	}

	slab_bufctl(slabp)[i-1] = BUFCTL_END;
	slabp->free = 0;
}
/* kmem_cache_alloc_one_tail: allocate one obj from a slab
 * @cachep: the cache obj alloc from
 * @slabp: the slab obj alloc from
 */
static inline void *kmem_cache_alloc_one_tail(kmem_cache_t *cachep,
			slab_t *slabp)
{
	void* objp;
	
	//set objp point to the first free obj in the slab
	//update the new num of used obj(free)
	slabp->inuse++;
	objp = slabp->s_mem + slabp->free*cachep->objsize;
	slabp->free = slab_bufctl(slabp)[slabp->free];

	//if free exceeds the end mark, move it from the partial list to the full
	if(slabp->free == BUFCTL_END){
		list_del(&slabp->list);
		list_add(&cachep->slabs_full, &slabp->list);
	}
	return objp;
}

/* kmem_cache_alloc_one: alloc a new slab from the list, 
 *  defined as a macro rather a inline function
 * @cachep: the cache slab allocate from
 */
#define kmem_cache_alloc_one(cachep)			\
({												\
	list_entry_t *slabs_partial, *entry;		\
 	slab_t *slabp;								\
												\
 	slabs_partial = &cachep->slabs_partial;		\
 	entry = list_next(slabs_partial);			\
 	if(entry == slabs_partial){		\
 		list_entry_t *slabs_free;				\
 		slabs_free = &cachep->slabs_free;		\
 		entry = list_next(slabs_free);			\
		if(entry == slabs_free)		\
 			goto alloc_new_slab;				\
 		list_del(entry);						\
 		list_add(slabs_partial, entry);			\
 	}											\
												\
 	slabp = list_str(entry, slab_t, list);		\
 	kmem_cache_alloc_one_tail(cachep, slabp);	\
})
/*__kmem_cache_alloc: main function to alloc object
 *@cachep: cache to allocate from
 *flags: flags need to specify
 */
//TODO here is somehow really a mess
static inline void *__kmem_cache_alloc(kmem_cache_t *cachep, uint32_t flags)
{
	bool save_flags;
	void* objp;

	//XXX for dma use check
	//kmem_cache_alloc_head(cachep, flags);

try_again:
	local_intr_save(save_flags);	//disable interrupt and save flags

	objp = kmem_cache_alloc_one(cachep);

	local_intr_restore(save_flags);
	return objp;
alloc_new_slab:

	local_intr_restore(save_flags);
	if(kmem_cache_grow(cachep, flags))
			goto try_again;
	return NULL;
}
/*kmem_cache_alloc: a simple function*/
void *kmem_cache_alloc(kmem_cache_t *cachep, uint32_t flags)
{
	return __kmem_cache_alloc(cachep, flags);
}

/*__kmem_cache_free: the main function to free a obj
 * @cachep: the cache free from
 * @objp: the obj to be freed
 */
void __kmem_cache_free(kmem_cache_t* cachep, void* objp)
{
	slab_t* slabp;
	//TODO add check
	slabp = GET_PAGE_SLAB(kva2page(objp));		//XXX maybe wrong

	uint32_t objnr = (objp-slabp->s_mem)/cachep->objsize;

	slab_bufctl(slabp)[objnr] = slabp->free;
	slabp->free = objnr;

	//adjust the array of slab in cache
	int inuse = slabp->inuse;
	if(!--slabp->inuse){
		list_del(&slabp->list);
		list_add(&cachep->slabs_free, &slabp->list);
	}
	else if(inuse == cachep->num){
		list_del(&slabp->list);
		list_add(&cachep->slabs_partial, &slabp->list);
	}
}

/*kmem_cache_free: free the obj from a cache
 * @cachep: the cache free from
 * @objp: the obj to be freed
 */
void kmem_cache_free(kmem_cache_t* cachep, void *objp)
{
	uint32_t flags;
	local_intr_save(flags);
	__kmem_cache_free(cachep, objp);
	local_intr_restore(flags);
}

/*kmalloc: function to alloc sizes obj without using cache_init
 * size: the size of obj needs to allocate.
 */
void* kmalloc(size_t size){
	//get the pointer to sizes_cache
	cache_sizes_t* csizep = cache_sizes;

	//find the suitable cache
	for(; csizep->cs_size; csizep++){
		if(size > csizep->cs_size)
			continue;
		return __kmem_cache_alloc(csizep->cs_cachep, 0);
	}
	return NULL;
}

/*kfree: function to free the object allocated by kmalloc
 * objp: the obj to be freed
 *TODO problem: if the page is empty in this cache, it will not return the page to the system, but rather kept it, which is undoubtedly wrong, so i slightly modified it to ensure that the page will be freed when the slab is free in sized cache
 */
void kfree(void* objp){
	kmem_cache_t* cachep;
	slab_t* slabp;
	uint32_t flags;

	if(!objp)
		return;
	
	local_intr_save(flags);
	cachep = GET_PAGE_CACHE(kva2page(objp));
	slabp = GET_PAGE_SLAB(kva2page(objp));
	__kmem_cache_free(cachep, objp);
	if(slabp->inuse == 0)
		kmem_slab_destroy(cachep, slabp);
	local_intr_restore(flags);
}
/*--------------------------------------------------
			init and downward interface
--------------------------------------------------*/
/*kmem_cache_init: init the chain linked list, the mutex and cache_cache color
 */
void kmem_cache_init(void)
{
	size_t left_over;
	
	slab_break_gfp_order = 0;		//TODO need some fix
	sem_init(&cache_chain_sem, 1);		//initiat the semaphore for access the cache_chain
	list_init(&cache_chain);	
	kmem_cache_estimate(0, cache_cache.objsize, 0,
			&left_over, &cache_cache.num);
	if(!cache_cache.num)
		BUG("Cache_init", "kmem_cache_t cannot be stored in a page.");

	
    list_init(&(cache_cache.slabs_full));
	list_init(&(cache_cache.slabs_partial));
	list_init(&(cache_cache.slabs_free));
	cache_cache.colour = left_over/cache_cache.colour_off;
	cache_cache.colour_next = 0;
}
/* kmem_cache_init: init sizes cache list, for kmalloc and kfree that haven't specific ctor
 * TODO I didn't figure out dma and up case here.
 */
void kmem_cache_sizes_init(){
	cache_sizes_t *sizes = cache_sizes;		//the pointer to the sizes cache array
	char name[20];

	memset(name, 0, sizeof(name));
	do{
		//allocate pages for sizes cache
		snprintf(name, sizeof(name), "size-%d", sizes->cs_size);
		if(!(sizes->cs_cachep = kmem_cache_create(name, sizes->cs_size, 0, SLAB_HWCACHE_ALIGN, NULL)))
			BUG("SIZES_CACHE INIT:", "cannot allocate sizes cache");
		
		if(!(sizes->cs_cachep->flags & CFGS_OFF_SLAB)){
			offslab_limit = sizes->cs_size - sizeof(slab_t);
			offslab_limit /= 2;
		}

		sizes++;
	}while(sizes->cs_size);
}
/*---------------------------------------------------
 * 				test and check part
 *-------------------------------------------------*/
#define DEBUG_SLAB 1

#if DEBUG_SLAB
static void slab_ctor(void* obj){
	slab_t* slabp = (slab_t*)obj;
	list_init(&(slabp->list));
    slabp->colouroff = L1_CACHE_BYTES;
	slabp->s_mem = (void*)0;
	slabp->inuse = 0;
	slabp->free = 0;	
}
/*slab_init: just call kmem_cache_init*/
void slab_init(void)
{
	//TODO no test for kmem_cache_init part of obj larger than a page
	kmem_cache_init();
	kmem_cache_sizes_init();
	int i = 0;
	
	kmem_cache_t* cache_1 = kmem_cache_create("slab_test", sizeof(slab_t), 0, SLAB_NO_REAP, slab_ctor);

	assert(list_empty(&cache_1->slabs_full));
	assert(list_empty(&cache_1->slabs_partial));
	assert(list_empty(&cache_1->slabs_free));
		
	slab_t* slab_1;
	for(;i<cache_1->num-1;i++){
		slab_1 = kmem_cache_alloc(cache_1, 0);
	}
	
	struct Page *p0, *p1;

	p0 = kva2page(slab_1);
	
	slab_t *slabp1, *slabp2;
	//allocate one less obj to make the slab full
	slabp1 = GET_PAGE_SLAB(p0);
	assert(!list_empty(&cache_1->slabs_partial)&&(&slabp1->list==list_next(&cache_1->slabs_partial)));
	//full slab
	slab_1 = kmem_cache_alloc(cache_1, 0);
	assert(!list_empty(&cache_1->slabs_full)&&(&slabp1->list==list_next(&cache_1->slabs_full)));
	assert(list_empty(&cache_1->slabs_partial));
	//a new page for slab
	slab_1 = kmem_cache_alloc(cache_1, 0);
	p0 = kva2page(slab_1);
	slabp1 = GET_PAGE_SLAB(p0);
	assert(!list_empty(&cache_1->slabs_full));
	assert(!list_empty(&cache_1->slabs_partial)&&(&slabp1->list==list_next(&cache_1->slabs_partial)));
	
	kmem_cache_t* cache_2 = kmem_cache_create("slab_test1", sizeof(slab_t), 0, SLAB_NO_REAP, slab_ctor);
	slab_t* slab_2, slab_3;
	
	slab_2 = kmem_cache_alloc(cache_2, 0);
	assert(list_next(&cache_2->next) == &cache_1->next);	
	p1 = kva2page(slab_2);
	slabp2 = GET_PAGE_SLAB(p1);
	void* pg = &slabp2->list;

	kmem_cache_free(cache_2, slab_2);
	assert(list_empty(&cache_2->slabs_partial)&&(pg == list_next(&cache_2->slabs_free)));
	
	slab_2 = kmem_cache_alloc(cache_2, 0);
	p1 = kva2page(slab_2);
	slabp2 = GET_PAGE_SLAB(p1);
	assert(list_empty(&cache_2->slabs_free)&&(&slabp2->list == list_next(&cache_2->slabs_partial)));
	kmem_cache_free(cache_2, slab_2);

	assert(kmem_cache_destroy(cache_2));
	cache_2 = kmem_cache_create("slab_test1", sizeof(slab_t), 0, SLAB_NO_REAP, slab_ctor);
	assert(list_next(&cache_2->next) == &cache_1->next);	

	//obj less than a page
	void* o_0, *o_1, *o_2;
	assert((o_0 = kmalloc(32))!=NULL);
	assert(((o_1 = kmalloc(32))!=NULL)&&(o_1 == o_0 + 32));	

	o_2 = kmalloc(32);
	kfree(o_1);
	//ensure no internal fragments in slab	
	assert(((o_1 = kmalloc(32))!=NULL)&&(o_1 == o_0 + 32));
	kfree(o_0);kfree(o_1);
	kfree(o_2);
	o_2 = kmalloc(32);
	//obj larger than a page
	o_0 = kmalloc(8190);
	o_1 = kmalloc(8192);
	assert(o_1 == o_0 + 8192);
	kfree(o_0);kfree(o_1);
	kfree(o_2);

	o_2 = kmalloc(64);
	kfree(o_2);
	kprintf("slab check succeeded!\n");
}
#endif
void BUG(const char* bug_location, char* bug_info){
    kprintf("%s:%s\n", bug_location, bug_info);
    assert(0);
}
