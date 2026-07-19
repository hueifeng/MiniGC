#include "pch.h"
#include "MiniGCHandleStore.h"
#include <iostream>

namespace
{
    constexpr int MaxHandles = 65535;
    int handlesCount = 0;
    OBJECTHANDLE handles[MaxHandles] = {};
    Object* dependentSecondaries[MaxHandles] = {};
    HandleType handleTypes[MaxHandles] = {};
    void* extraInfo[MaxHandles] = {};

    int GetHandleIndex(OBJECTHANDLE handle)
    {
        if (handle == nullptr)
            return -1;

        uintptr_t address = reinterpret_cast<uintptr_t>(handle);
        uintptr_t begin = reinterpret_cast<uintptr_t>(&handles[0]);
        uintptr_t end = reinterpret_cast<uintptr_t>(&handles[MaxHandles]);
        if (address < begin || address >= end || ((address - begin) % sizeof(handles[0])) != 0)
            return -1;

        return static_cast<int>((address - begin) / sizeof(handles[0]));
    }

    bool IsStrongHandle(HandleType type)
    {
        return type == HNDTYPE_STRONG || type == HNDTYPE_PINNED ||
            type == HNDTYPE_ASYNCPINNED || type == HNDTYPE_SIZEDREF;
    }
}

void MiniGCHandleStore::Uproot()
{
    std::cout << " MiniGCHandleStore::Uproot" << std::endl;
}

bool MiniGCHandleStore::ContainsHandle(OBJECTHANDLE handle)
{
    int index = GetHandleIndex(handle);
    return index >= 0 && index < handlesCount && handles[index] != nullptr;
}

OBJECTHANDLE MiniGCHandleStore::CreateHandleOfType(Object* object, HandleType type)
{
    if (handlesCount >= MaxHandles)
        return nullptr;

    int index = handlesCount++;
    handles[index] = (OBJECTHANDLE__*)object;
    handleTypes[index] = type;
    dependentSecondaries[index] = nullptr;
    extraInfo[index] = nullptr;
    return (OBJECTHANDLE)&handles[index];
}

OBJECTHANDLE MiniGCHandleStore::CreateHandleOfType(Object* object, HandleType type, int heapToAffinitizeTo)
{
    return CreateHandleOfType(object, type);
}

OBJECTHANDLE MiniGCHandleStore::CreateHandleWithExtraInfo(Object* object, HandleType type, void* pExtraInfo)
{
    OBJECTHANDLE handle = CreateHandleOfType(object, type);
    SetExtraInfo(handle, pExtraInfo);
    return handle;
}

OBJECTHANDLE MiniGCHandleStore::CreateDependentHandle(Object* primary, Object* secondary)
{
    OBJECTHANDLE handle = CreateHandleOfType(primary, HNDTYPE_DEPENDENT);
    SetDependentSecondary(handle, secondary);
    return handle;
}

void MiniGCHandleStore::ScanHandles(promote_func* pf, ScanContext* sc)
{
    for (int i = 0; i < handlesCount; ++i)
    {
        if (handles[i] != nullptr && IsStrongHandle(handleTypes[i]))
        {
            pf((Object**)&handles[i], sc, 0);
        }
    }
}

void MiniGCHandleStore::DestroyHandle(OBJECTHANDLE handle)
{
    int index = GetHandleIndex(handle);
    if (index < 0 || index >= handlesCount)
        return;

    handles[index] = nullptr;
    dependentSecondaries[index] = nullptr;
    extraInfo[index] = nullptr;
}

Object* MiniGCHandleStore::GetObject(OBJECTHANDLE handle) const
{
    int index = GetHandleIndex(handle);
    return index >= 0 && index < handlesCount ? (Object*)handles[index] : nullptr;
}

Object* MiniGCHandleStore::GetDependentSecondary(OBJECTHANDLE handle) const
{
    int index = GetHandleIndex(handle);
    return index >= 0 && index < handlesCount ? dependentSecondaries[index] : nullptr;
}

void MiniGCHandleStore::SetDependentSecondary(OBJECTHANDLE handle, Object* object)
{
    int index = GetHandleIndex(handle);
    if (index >= 0 && index < handlesCount)
        dependentSecondaries[index] = object;
}

HandleType MiniGCHandleStore::GetType(OBJECTHANDLE handle) const
{
    int index = GetHandleIndex(handle);
    return index >= 0 && index < handlesCount ? handleTypes[index] : HNDTYPE_WEAK_SHORT;
}

void MiniGCHandleStore::SetExtraInfo(OBJECTHANDLE handle, void* value)
{
    int index = GetHandleIndex(handle);
    if (index >= 0 && index < handlesCount)
        extraInfo[index] = value;
}

void* MiniGCHandleStore::GetExtraInfo(OBJECTHANDLE handle) const
{
    int index = GetHandleIndex(handle);
    return index >= 0 && index < handlesCount ? extraInfo[index] : nullptr;
}
