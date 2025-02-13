#include "common_lib.h"
#include "lock_common.h"

#ifndef _COMMONLIB_NO_LOCKS_

PFUNC_LockInit           LockInit = NULL;

PFUNC_LockAcquire        LockAcquire = NULL;

PFUNC_LockTryAcquire     LockTryAcquire = NULL;

PFUNC_LockRelease        LockRelease = NULL;

PFUNC_LockIsOwner        LockIsOwner = NULL;

void
LockSystemInit(
    IN      BOOLEAN             MonitorSupport
    )
{

// warning C4028: formal parameter 1 different from declaration
#pragma warning(disable:4028)
#pragma warning(disable:4113) // Error for VS2022 - same warning, different error code

    if (MonitorSupport)
    {
        // we have monitor support
        LockInit = MonitorLockInit;
        LockAcquire = MonitorLockAcquire;
        LockTryAcquire = MonitorLockTryAcquire;
        LockIsOwner = MonitorLockIsOwner;
        LockRelease = MonitorLockRelease;
    }
    else
    {
        // use classic spinlock
        LockInit = SpinlockInit;
        LockAcquire = SpinlockAcquire;
        LockTryAcquire = SpinlockTryAcquire;
        LockIsOwner = SpinlockIsOwner;
        LockRelease = SpinlockRelease;
    }
#pragma warning(default:4028)
}

#endif // _COMMONLIB_NO_LOCKS_
