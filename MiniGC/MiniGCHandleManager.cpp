#include "pch.h"
#include "MiniGCHandleManager.h"
#include <iostream>
#include "MiniGCHandleStore.h"

MiniGCHandleStore* g_miniGCHandleStore;

bool MiniGCHandleManager::Initialize()
{
	g_miniGCHandleStore = new MiniGCHandleStore();
	std::cout << "MiniGCHandleManager Initialize" << std::endl;
	return true;
}

void MiniGCHandleManager::Shutdown()
{
}

IGCHandleStore* MiniGCHandleManager::GetGlobalHandleStore()
{
	return g_miniGCHandleStore;
}

IGCHandleStore* MiniGCHandleManager::CreateHandleStore()
{
	return g_miniGCHandleStore;
}

void MiniGCHandleManager::DestroyHandleStore(IGCHandleStore* store)
{
}

OBJECTHANDLE MiniGCHandleManager::CreateGlobalHandleOfType(Object* object, HandleType type)
{
	return g_miniGCHandleStore->CreateHandleOfType(object, type);
}

OBJECTHANDLE MiniGCHandleManager::CreateDuplicateHandle(OBJECTHANDLE handle)
{
	return OBJECTHANDLE();
}

void MiniGCHandleManager::DestroyHandleOfType(OBJECTHANDLE handle, HandleType type)
{
	
}

void MiniGCHandleManager::DestroyHandleOfUnknownType(OBJECTHANDLE handle)
{
	 
}

void MiniGCHandleManager::SetExtraInfoForHandle(OBJECTHANDLE handle, HandleType type, void* pExtraInfo)
{
}

void* MiniGCHandleManager::GetExtraInfoFromHandle(OBJECTHANDLE handle)
{
	return nullptr;
}

void MiniGCHandleManager::StoreObjectInHandle(OBJECTHANDLE handle, Object* object)
{
	Object** handleObj = (Object**)handle;
	*handleObj = object;
}

bool MiniGCHandleManager::StoreObjectInHandleIfNull(OBJECTHANDLE handle, Object* object)
{
	Object** handleObj = (Object**)handle;
	if (*handleObj == NULL)
	{
		*handleObj = object;
		return true;
	}
	return false;
}

void MiniGCHandleManager::SetDependentHandleSecondary(OBJECTHANDLE handle, Object* object)
{
}

Object* MiniGCHandleManager::GetDependentHandleSecondary(OBJECTHANDLE handle)
{
	return nullptr;
}

Object* MiniGCHandleManager::InterlockedCompareExchangeObjectInHandle(OBJECTHANDLE handle, Object* object, Object* comparandObject)
{
	Object** handleObject = (Object**)handle;
	if (*handleObject == comparandObject)
	{
		*handleObject = object;
	}
	return *handleObject;
}

HandleType MiniGCHandleManager::HandleFetchType(OBJECTHANDLE handle)
{
	return HandleType();
}

void MiniGCHandleManager::TraceRefCountedHandles(HANDLESCANPROC callback, uintptr_t param1, uintptr_t param2)
{
}

void MiniGCHandleManager::ScanHandles(promote_func* pf, ScanContext* sc)
{
	g_miniGCHandleStore->ScanHandles(pf, sc);
}

