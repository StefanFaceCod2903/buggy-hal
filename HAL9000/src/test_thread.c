#include "test_common.h"
#include "test_thread.h"
#include "test_timer.h"
#include "test_priority_scheduler.h"
#include "test_priority_donation.h"

#include "mutex.h"


FUNC_ThreadStart                TestThreadYield;

FUNC_ThreadStart                TestMutexes;

FUNC_ThreadStart                TestCpuIntense;

static FUNC_ThreadPrepareTest   _ThreadTestPassContext;

const THREAD_TEST THREADS_TEST[] =
{
    // Tests just for fun
    { "ThreadYield", TestThreadYield, NULL, NULL, NULL, NULL, FALSE, FALSE },
    { "Mutex", TestMutexes, TestPrepareMutex, (PVOID) FALSE, NULL, NULL, FALSE, FALSE },
    { "CpuIntense", TestCpuIntense, NULL, NULL, NULL, NULL, FALSE, FALSE },

    // Actual tests used for validating the project

    // Timer
    {   "TestThreadTimerAbsolute", TestThreadTimerSleep,
        _ThreadTestPassContext, (PVOID)TEST_TIMER_ABSOLUTE, NULL, NULL,
        ThreadPriorityDefault, FALSE, TRUE, FALSE},

    {   "TestThreadTimerAbsolutePassed", TestThreadTimerSleep,
        _ThreadTestPassContext, (PVOID)TEST_TIMER_ABSOLUTE_PASSED, NULL, NULL,
        ThreadPriorityDefault, FALSE, TRUE, FALSE},

    {   "TestThreadTimerPeriodicMultiple", TestThreadTimerSleep,
        _ThreadTestPassContext, (PVOID)TEST_TIMER_PERIODIC_MULTIPLE, NULL, NULL,
        ThreadPriorityDefault, FALSE, TRUE, FALSE},

    {   "TestThreadTimerPeriodicOnce", TestThreadTimerSleep,
        _ThreadTestPassContext, (PVOID)TEST_TIMER_PERIODIC_ONCE, NULL, NULL,
        ThreadPriorityDefault, FALSE, TRUE, FALSE},

    {   "TestThreadTimerPeriodicZero", TestThreadTimerSleep,
        _ThreadTestPassContext, (PVOID)TEST_TIMER_PERIODIC_ZERO, NULL, NULL,
        ThreadPriorityDefault, FALSE, TRUE, FALSE},

    {   "TestThreadTimerMultipleThreads", TestThreadTimerMultiple,
        TestThreadTimerPrepare, (PVOID)FALSE, NULL, NULL,
        ThreadPriorityDefault, FALSE, FALSE, FALSE},

    {   "TestThreadTimerMultipleTimers", TestThreadTimerMultiple,
        TestThreadTimerPrepare, (PVOID)TRUE, NULL, TestThreadTimerPostFinish,
        ThreadPriorityDefault, FALSE, FALSE, TRUE},

    {   "TestThreadTimerRelative", TestThreadTimerSleep,
        _ThreadTestPassContext, (PVOID)TEST_TIMER_RELATIVE, NULL, NULL,
        ThreadPriorityDefault, FALSE, TRUE, FALSE},

    {   "TestThreadTimerRelativeZero", TestThreadTimerSleep,
        _ThreadTestPassContext, (PVOID)TEST_TIMER_RELATIVE_ZERO, NULL, NULL,
        ThreadPriorityDefault, FALSE, TRUE, FALSE},

    // Priority Scheduler
    {   "TestThreadPriorityWakeup", TestThreadPriorityWakeup,
        TestThreadPrepareWakeupEvent, NULL, TestThreadPostCreateWakeup, NULL,
        ThreadPriorityLowest, TRUE, FALSE, FALSE},

    {   "TestThreadPriorityMutex", TestThreadPriorityMutex,
        TestPrepareMutex, (PVOID)TRUE, TestThreadPostCreateMutex, NULL,
        ThreadPriorityLowest, TRUE, FALSE, FALSE},

    {   "TestThreadPriorityYield", TestThreadPriorityExecution,
        _ThreadTestPassContext, (PVOID) FALSE, NULL, NULL,
        ThreadPriorityMaximum, FALSE, TRUE, FALSE},

    {   "TestThreadPriorityRoundRobin", TestThreadPriorityExecution,
        _ThreadTestPassContext, (PVOID) TRUE, NULL, NULL,
        ThreadPriorityMaximum, FALSE, FALSE, FALSE},

    // Priority Donation
    {   "TestThreadPriorityDonationOnce", TestThreadPriorityDonationBasic,
        _ThreadTestPassContext, (PVOID) FALSE, NULL, NULL,
        ThreadPriorityDefault, FALSE, TRUE, FALSE},

    {   "TestThreadPriorityDonationLower", TestThreadPriorityDonationBasic,
        _ThreadTestPassContext, (PVOID)TRUE, NULL, NULL,
        ThreadPriorityDefault, FALSE, TRUE, FALSE},

    {   "TestThreadPriorityDonationMulti", TestThreadPriorityDonationMultiple,
        _ThreadTestPassContext, (PVOID) TestPriorityDonationMultipleOneThreadPerLock, NULL, NULL,
        ThreadPriorityDefault, FALSE, TRUE, FALSE},

    {   "TestThreadPriorityDonationMultiInverted", TestThreadPriorityDonationMultiple,
        _ThreadTestPassContext, (PVOID) TestPriorityDonationMultipleOneThreadPerLockInverseRelease, NULL, NULL,
        ThreadPriorityDefault, FALSE, TRUE, FALSE},

    {   "TestThreadPriorityDonationMultiThreads", TestThreadPriorityDonationMultiple,
        _ThreadTestPassContext, (PVOID)TestPriorityDonationMultipleTwoThreadsPerLock, NULL, NULL,
        ThreadPriorityDefault, FALSE, TRUE, FALSE},

    {   "TestThreadPriorityDonationMultiThreadsInverted", TestThreadPriorityDonationMultiple,
        _ThreadTestPassContext, (PVOID)TestPriorityDonationMultipleTwoThreadsPerLockInverseRelease, NULL, NULL,
        ThreadPriorityDefault, FALSE, TRUE, FALSE},

    {   "TestThreadPriorityDonationNest", TestThreadPriorityDonationChain,
        _ThreadTestPassContext, (PVOID) 3, NULL, NULL,
        ThreadPriorityDefault, FALSE, TRUE, FALSE},

    {   "TestThreadPriorityDonationChain", TestThreadPriorityDonationChain,
        _ThreadTestPassContext, (PVOID) 7, NULL, NULL,
        ThreadPriorityDefault, FALSE, TRUE, FALSE},
};

const DWORD THREADS_TOTAL_NO_OF_TESTS = ARRAYSIZE(THREADS_TEST);

#define CPU_INTENSE_MEMORY_SIZE         PAGE_SIZE

typedef struct _TEST_THREAD_INFO
{
    PTHREAD                             Thread;
    STATUS                              Status;
} TEST_THREAD_INFO, *PTEST_THREAD_INFO;

void
TestThreadFunctionality(
    IN      THREAD_TEST*                ThreadTest,
    IN_OPT  PVOID                       ContextForTestFunction,
    IN      DWORD                       NumberOfThreads
    )
{
    DWORD i;
    STATUS status;
    char threadName[MAX_PATH];
    PTEST_THREAD_INFO pThreadsInfo;
    PVOID pCtx;
    THREAD_PRIORITY currentPriority;
    BYTE priorityIncrement;
    DWORD noOfThreads;

    LOG_FUNC_START_THREAD;

    UNREFERENCED_PARAMETER(ContextForTestFunction);

    ASSERT(NULL != ThreadTest);
    ASSERT(NULL != ThreadTest->ThreadFunction);
    ASSERT(NULL != ThreadTest->TestName);

    pThreadsInfo = NULL;
    pCtx = NULL;
    currentPriority = ThreadTest->BasePriority;
    priorityIncrement = ThreadTest->IncrementPriorities ? 1 : 0;

    noOfThreads = ThreadTest->IgnoreThreadCount ? 1 : NumberOfThreads;
    ASSERT(noOfThreads > 0);

    pThreadsInfo = ExAllocatePoolWithTag(PoolAllocatePanicIfFail | PoolAllocateZeroMemory,
                                         sizeof(TEST_THREAD_INFO) * noOfThreads,
                                         HEAP_TEST_TAG,
                                         0);

    if (NULL != ThreadTest->ThreadPrepareFunction)
    {
        ThreadTest->ThreadPrepareFunction(&pCtx, noOfThreads, ThreadTest->PrepareFunctionContext);
    }

    LOG_TEST_LOG("Test [%s] START!\n", ThreadTest->TestName);
    LOG_TEST_LOG("Will create %d threads for running test [%s]\n", noOfThreads, ThreadTest->TestName);
    for (i = 0; i < noOfThreads; ++i)
    {
        snprintf(threadName, MAX_PATH, "%s-%02x", ThreadTest->TestName, i );
        status = ThreadCreate(threadName,
                              currentPriority,
                              ThreadTest->ThreadFunction,
                              ThreadTest->ArrayOfContexts ? ((PVOID*) pCtx)[i] : pCtx,
                              &pThreadsInfo[i].Thread );
        ASSERT(SUCCEEDED(status));

        // increment priority with wrap around
        currentPriority = (currentPriority + priorityIncrement) % ThreadPriorityReserved;
    }

    if (ThreadTest->ThreadPostCreateFunction != NULL)
    {
        ThreadTest->ThreadPostCreateFunction(pCtx);
    }

    for (i = 0; i < noOfThreads; ++i)
    {
        ThreadWaitForTermination(pThreadsInfo[i].Thread, &pThreadsInfo[i].Status);

        LOG_TEST_LOG("Thread [%s] terminated with status: 0x%x\n",
                     ThreadGetName(pThreadsInfo[i].Thread), pThreadsInfo[i].Status);

        ThreadCloseHandle(pThreadsInfo[i].Thread);
        pThreadsInfo[i].Thread = NULL;
    }

    if (ThreadTest->ThreadPostFinishFunction != NULL)
    {
        ThreadTest->ThreadPostFinishFunction(pCtx, noOfThreads);
    }

    LOG_TEST_LOG("Test [%s] END!\n", ThreadTest->TestName);

    if (pCtx != NULL)
    {
        if (ThreadTest->ArrayOfContexts)
        {
            for (i = 0; i < noOfThreads; ++i)
            {
                ExFreePoolWithTag(((PVOID*)pCtx)[i], HEAP_TEST_TAG);
                ((PVOID*)pCtx)[i] = NULL;
            }
        }

        ExFreePoolWithTag(pCtx, HEAP_TEST_TAG);
        pCtx = NULL;
    }

    ASSERT(NULL != pThreadsInfo)
    ExFreePoolWithTag(pThreadsInfo, HEAP_TEST_TAG);
    pThreadsInfo = NULL;

    LOG_FUNC_END_THREAD;
}

void
TestAllThreadFunctionalities(
    IN      DWORD                       NumberOfThreads
    )
{
    for (DWORD i = 0; i < THREADS_TOTAL_NO_OF_TESTS; ++i)
    {
        TestThreadFunctionality(&THREADS_TEST[i], NULL, NumberOfThreads );
    }
}

STATUS
(__cdecl TestThreadYield)(
    IN_OPT      PVOID       Context
    )
{
    DWORD i,j;

    LOG_FUNC_START_THREAD;

    UNREFERENCED_PARAMETER(Context);

    for (i = 0; i < 0x100; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            printf("[%d]This is cool\n", i);
        }
        ThreadYield();
    }

    LOG_FUNC_END_THREAD;

    return STATUS_SUCCESS;
}

STATUS
(__cdecl TestMutexes)(
    IN_OPT      PVOID       Context
    )
{
    PMUTEX pMutex = (PMUTEX) Context;
    static QWORD __value = 0;
    DWORD i;

    ASSERT( NULL != Context );
    UNREFERENCED_PARAMETER(pMutex);


    for (i = 0; i < 0x10000; ++i)
    {
        MutexAcquire(pMutex);
        __value++;
        MutexRelease(pMutex);
    }

    LOGTPL("Value: 0x%x\n", __value );

    return STATUS_SUCCESS;
}

STATUS
(__cdecl TestCpuIntense)(
    IN_OPT      PVOID       Context
    )
{
    volatile BYTE* pMemory;

    ASSERT( NULL == Context );

    pMemory = ExAllocatePoolWithTag(PoolAllocatePanicIfFail | PoolAllocateZeroMemory, CPU_INTENSE_MEMORY_SIZE, HEAP_TEST_TAG, 0 );

    for (DWORD j = 0; j < 0x10000; ++j)
    {
        for (DWORD i = 0; i < CPU_INTENSE_MEMORY_SIZE; ++i)
        {
            pMemory[i] = pMemory[i] ^ 0x63;
        }
    }

    ExFreePoolWithTag((PVOID)pMemory, HEAP_TEST_TAG);

    return STATUS_SUCCESS;
}

static
void
(__cdecl _ThreadTestPassContext)(
    OUT_OPT_PTR     PVOID*              Context,
    IN              DWORD               NumberOfThreads,
    IN              PVOID               PrepareContext
    )
{
    #pragma warning(disable:4090)
    PVOID pNewContext;
    ASSERT(NULL != Context);

    UNREFERENCED_PARAMETER(NumberOfThreads);

    pNewContext = ExAllocatePoolWithTag(PoolAllocatePanicIfFail, sizeof(PVOID), HEAP_TEST_TAG, 0);

    memcpy(pNewContext, &PrepareContext, sizeof(PVOID));

    *Context = pNewContext;
}