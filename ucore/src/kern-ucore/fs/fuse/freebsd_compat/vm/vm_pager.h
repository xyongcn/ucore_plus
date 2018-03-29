#ifndef _FREEBSD_COMPAT_VM_PAGER_H_
#define	_FREEBSD_COMPAT_VM_PAGER_H_

/*
 * get/put return values
 * OK    operation was successful
 * BAD   specified data was out of the accepted range
 * FAIL  specified data was in range, but doesn't exist
 * PEND  operations was initiated but not completed
 * ERROR error while accessing data that is in range and exists
 * AGAIN temporary resource shortage prevented operation from happening
 */
#define VM_PAGER_OK     0
#define VM_PAGER_BAD    1
#define VM_PAGER_FAIL   2
#define VM_PAGER_PEND   3
#define VM_PAGER_ERROR  4
#define VM_PAGER_AGAIN  5

#define VM_PAGER_PUT_SYNC               0x0001
#define VM_PAGER_PUT_INVAL              0x0002
#define VM_PAGER_CLUSTER_OK             0x0008

#endif
