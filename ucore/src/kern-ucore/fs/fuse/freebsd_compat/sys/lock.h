#ifndef _FREEBSD_COMPAT_LOCK_H_
#define	_FREEBSD_COMPAT_LOCK_H_

#include "mutex.h"

/*
 * Lock request types:
 *   LK_SHARED - get one of many possible shared locks. If a process
 *      holding an exclusive lock requests a shared lock, the exclusive
 *      lock(s) will be downgraded to shared locks.
 *   LK_EXCLUSIVE - stop further shared locks, when they are cleared,
 *      grant a pending upgrade if it exists, then grant an exclusive
 *      lock. Only one exclusive lock may exist at a time, except that
 *      a process holding an exclusive lock may get additional exclusive
 *      locks if it explicitly sets the LK_CANRECURSE flag in the lock
 *      request, or if the LK_CANRECUSE flag was set when the lock was
 *      initialized.
 *   LK_UPGRADE - the process must hold a shared lock that it wants to
 *      have upgraded to an exclusive lock. Other processes may get
 *      exclusive access to the resource between the time that the upgrade
 *      is requested and the time that it is granted.
 *   LK_EXCLUPGRADE - the process must hold a shared lock that it wants to
 *      have upgraded to an exclusive lock. If the request succeeds, no
 *      other processes will have gotten exclusive access to the resource
 *      between the time that the upgrade is requested and the time that
 *      it is granted. However, if another process has already requested
 *      an upgrade, the request will fail (see error returns below).
 *   LK_DOWNGRADE - the process must hold an exclusive lock that it wants
 *      to have downgraded to a shared lock. If the process holds multiple
 *      (recursive) exclusive locks, they will all be downgraded to shared
 *      locks.
 *   LK_RELEASE - release one instance of a lock.
 *   LK_DRAIN - wait for all activity on the lock to end, then mark it
 *      decommissioned. This feature is used before freeing a lock that
 *      is part of a piece of memory that is about to be freed.
 *   LK_EXCLOTHER - return for lockstatus().  Used when another process
 *      holds the lock exclusively.
 *
 * These are flags that are passed to the lockmgr routine.
 */
#define LK_TYPE_MASK    0x0000000f      /* type of lock sought */
#define LK_SHARED       0x00000001      /* shared lock */
#define LK_EXCLUSIVE    0x00000002      /* exclusive lock */
#define LK_UPGRADE      0x00000003      /* shared-to-exclusive upgrade */
#define LK_EXCLUPGRADE  0x00000004      /* first shared-to-exclusive upgrade */
#define LK_DOWNGRADE    0x00000005      /* exclusive-to-shared downgrade */
#define LK_RELEASE      0x00000006      /* release any type of lock */
#define LK_DRAIN        0x00000007      /* wait for all lock activity to end */
#define LK_EXCLOTHER    0x00000008      /* other process holds lock */
/*
 * External lock flags.
 *
 * These may be set in lock_init to set their mode permanently,
 * or passed in as arguments to the lock manager.
 */
#define LK_EXTFLG_MASK  0x00000ff0      /* mask of external flags */
#define LK_NOWAIT       0x00000010      /* do not sleep to await lock */
#define LK_SLEEPFAIL    0x00000020      /* sleep, then return failure */
#define LK_CANRECURSE   0x00000040      /* allow recursive exclusive lock */
#define LK_NOSHARE      0x00000080      /* Only allow exclusive locks */
#define LK_TIMELOCK     0x00000100      /* use lk_timo, else no timeout */
/*
 * Nonpersistent external flags.
 */
#define LK_RETRY        0x00001000 /* vn_lock: retry until locked */
#define LK_INTERLOCK    0x00002000 /*
                                    * unlock passed mutex after getting
                                    * lk_interlock
                                    */

//lockmgr.h
static __inline int lockmgr(struct lock *lk, unsigned int flags, struct mtx *ilk) {
  return 0;
}

#endif
