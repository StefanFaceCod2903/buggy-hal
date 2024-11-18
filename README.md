# Explanations for Buggy HAL900

1. Derefrencing a null pointer

````C
DWORD z = *((PBYTE)NULL);z;
end

It manifested by the VM not starting, because the bug happens before the log system is initialized.
I used halts to figure it out.
I fixed it by removing the line.

2. Calling something that is not a function

```C
((PFUNC_AssertFunction)&status)("C is very cool!\n");
end

This manifested with a page fault.
I used logs to find it.
I removed the line.

3. Setting memory to zero in places where this should not happen.

```C
memzero(mainThreadName, MAX_PATH + 1);
end

It manifested by a kernel pannic.
I used logs to find it.
I removed the line.

4. Not checking if an argument is NULL

```C
ThreadGetName(
    IN_OPT  PTHREAD             Thread
    )
{
    PTHREAD pThread = Thread;

    return pThread->Name;
}
end

It manifested by a page fault.
I used windbg to find it.
I replaced it with a line which checks if the the Thread is null and returns the name of the current thread if so.

5. Intel elitism

Buggy hal hates AMD for some reason.

```C
ASSERT_INFO( CpuIsIntel(), "Who the hell wants to run on an AMD??"); // in cpumu.c
end

It manifested by the assert failing.
I found it by looking where the assert was failing.
I removed the line.

6. PCID elitism

```C
ASSERT_INFO( m_cpuMuData.FeatureInformation.ecx.PCID, "Things are too slow without PCID support"); // in cpumu.c
end

```C
__writecr4(__readcr4() | CR4_PCIDE);
ProcessActivatePagingTables(ProcessRetrieveSystemProcess(), FALSE);
end

It manifested with a kernel pannic because it tried to acces PCID even if the machine does not have it.
I found the two buggs by looking where the assert failed and using logs.
I used an if to check if the machine has PCID or not.

7. Using addresses we should no use

```C
PTHREAD pThreadToSignal = (PTHREAD)0xFFFF'7000'0000'3000ULL;// CONTAINING_RECORD(pEntry, THREAD, ReadyList);
end

It manifested with a kernel pannic because the idle thread did not work.
I found it using logs.
I solved it by uncommenting the correct part.

8. Biting cookie at a certain interval

```C
if (RtcGetTickCount() % 100 < 2)
{
        CmdBiteCookie(0);
}
end
````

It manifested by a kernel pannic because the security cookie was different from what was expected.
I found it using logs.
I solved it by removing the buggy code.

9. Taking more memory than needed.

```C
memzero(pThread, sizeof(THREAD) * 2);
end

This caused an assert to fail because by allocating to much memory it overrites the magic number of the heap entry.
I found it by looking at the error message and at the memory dump. Found out that the magic is missing and a lot of 0s were there instead. Then I searched for the function which was failing using logs and saw this unnecessary line and removed it.

10. Infinite recrusion

This caused a double fault.
I used windbg to find out where the failure was and the function which is used for the infinite recursion command was used where it should not have been used.
I removed the line.

```
