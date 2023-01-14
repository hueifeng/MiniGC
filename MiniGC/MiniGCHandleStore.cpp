#include "pch.h"
#include "MiniGCHandleStore.h"
#include <iostream>

int handlesCount = 0;
OBJECTHANDLE handles[65535];

void MiniGCHandleStore::Uproot()
{
    std::cout << " MiniGCHandleStore::Uproot" << std::endl;
}

bool MiniGCHandleStore::ContainsHandle(OBJECTHANDLE handle)
{
  
    return false;
}

OBJECTHANDLE MiniGCHandleStore::CreateHandleOfType(Object* object, HandleType type)
{
    handles[handlesCount] = (OBJECTHANDLE__*)object;
    return (OBJECTHANDLE)&handles[handlesCount++];
    std::cout << " MiniGCHandleStore::CreateHandleOfType" << std::endl;
    return OBJECTHANDLE();
}

OBJECTHANDLE MiniGCHandleStore::CreateHandleOfType(Object* object, HandleType type, int heapToAffinitizeTo)
{
    std::cout << " MiniGCHandleStore::CreateHandleOfType" << std::endl;
    return OBJECTHANDLE();
}

OBJECTHANDLE MiniGCHandleStore::CreateHandleWithExtraInfo(Object* object, HandleType type, void* pExtraInfo)
{
    std::cout << " MiniGCHandleStore::CreateHandleWithExtraInfo" << std::endl;
    return OBJECTHANDLE();
}

OBJECTHANDLE MiniGCHandleStore::CreateDependentHandle(Object* primary, Object* secondary)
{
    std::cout << " MiniGCHandleStore::CreateDependentHandle" << std::endl;
    handles[handlesCount] = (OBJECTHANDLE__*)primary;
    return (OBJECTHANDLE)&handles[handlesCount++];
}

void MiniGCHandleStore::ScanHandles(promote_func* pf, ScanContext* sc)
{
    for (int i = 0; i < handlesCount; ++i)
    {
        if (handles[i] != nullptr)
        {
            pf((Object**)&handles[i], sc, 0);
        }
    }
}
