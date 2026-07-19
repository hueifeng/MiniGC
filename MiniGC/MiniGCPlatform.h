#pragma once

#if defined(_WIN32)
#include <windows.h>

#define MINIGC_EXPORT extern "C" __declspec(dllexport)

inline void* MiniGCAllocatePages(size_t size)
{
    return VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

inline bool MiniGCFreePages(void* address, size_t /*size*/)
{
    return VirtualFree(address, 0, MEM_RELEASE) != 0;
}

inline void* MiniGCCompareExchangePointer(void* volatile* target, void* exchange, void* comparand)
{
    return InterlockedCompareExchangePointer(target, exchange, comparand);
}
#else
#ifndef TARGET_UNIX
#define TARGET_UNIX 1
#endif

#include <sys/mman.h>

#define MINIGC_EXPORT extern "C" __attribute__((visibility("default")))

inline void* MiniGCAllocatePages(size_t size)
{
    void* result = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    return result == MAP_FAILED ? nullptr : result;
}

inline bool MiniGCFreePages(void* address, size_t size)
{
    return address != nullptr && munmap(address, size) == 0;
}

inline void* MiniGCCompareExchangePointer(void* volatile* target, void* exchange, void* comparand)
{
    void* expected = comparand;
    __atomic_compare_exchange_n(target, &expected, exchange, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return expected;
}
#endif
