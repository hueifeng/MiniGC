#include "pch.h"
#include "MiniGCHandleManager.h"
#include <iostream>
#include "MiniGCHandleStore.h"

MiniGCHandleStore* g_miniGCHandleStore;

bool MiniGCHandleManager::Initialize()
{
	if (g_miniGCHandleStore == nullptr)
		g_miniGCHandleStore = new MiniGCHandleStore();
	std::cout << "MiniGCHandleManager Initialize" << std::endl;
	return true;
}

void MiniGCHandleManager::Shutdown()
{
	delete g_miniGCHandleStore;
	g_miniGCHandleStore = nullptr;
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
	return g_miniGCHandleStore == nullptr ? nullptr : g_miniGCHandleStore->CreateHandleOfType(object, type);
}

OBJECTHANDLE MiniGCHandleManager::CreateDuplicateHandle(OBJECTHANDLE handle)
{
	if (g_miniGCHandleStore == nullptr || !g_miniGCHandleStore->ContainsHandle(handle))
		return nullptr;
	return g_miniGCHandleStore->CreateHandleOfType(
		g_miniGCHandleStore->GetObject(handle), g_miniGCHandleStore->GetType(handle));
}

void MiniGCHandleManager::DestroyHandleOfType(OBJECTHANDLE handle, HandleType type)
{
	if (g_miniGCHandleStore != nullptr)
		g_miniGCHandleStore->DestroyHandle(handle);
}

void MiniGCHandleManager::DestroyHandleOfUnknownType(OBJECTHANDLE handle)
{
	if (g_miniGCHandleStore != nullptr)
		g_miniGCHandleStore->DestroyHandle(handle);
}

void MiniGCHandleManager::SetExtraInfoForHandle(OBJECTHANDLE handle, HandleType type, void* pExtraInfo)
{
	if (g_miniGCHandleStore != nullptr)
		g_miniGCHandleStore->SetExtraInfo(handle, pExtraInfo);
}

void* MiniGCHandleManager::GetExtraInfoFromHandle(OBJECTHANDLE handle)
{
	return g_miniGCHandleStore == nullptr ? nullptr : g_miniGCHandleStore->GetExtraInfo(handle);
}

void MiniGCHandleManager::StoreObjectInHandle(OBJECTHANDLE handle, Object* object)
{
	if (g_miniGCHandleStore == nullptr || !g_miniGCHandleStore->ContainsHandle(handle))
		return;
	Object** handleObj = (Object**)handle;
	*handleObj = object;
}

bool MiniGCHandleManager::StoreObjectInHandleIfNull(OBJECTHANDLE handle, Object* object)
{
	if (g_miniGCHandleStore == nullptr || !g_miniGCHandleStore->ContainsHandle(handle))
		return false;
	Object** handleObj = (Object**)handle;
	return MiniGCCompareExchangePointer(
		reinterpret_cast<void* volatile*>(handleObj), object, nullptr) == nullptr;
}

void MiniGCHandleManager::SetDependentHandleSecondary(OBJECTHANDLE handle, Object* object)
{
	if (g_miniGCHandleStore != nullptr)
		g_miniGCHandleStore->SetDependentSecondary(handle, object);
}

Object* MiniGCHandleManager::GetDependentHandleSecondary(OBJECTHANDLE handle)
{
	return g_miniGCHandleStore == nullptr ? nullptr : g_miniGCHandleStore->GetDependentSecondary(handle);
}

Object* MiniGCHandleManager::InterlockedCompareExchangeObjectInHandle(OBJECTHANDLE handle, Object* object, Object* comparandObject)
{
	if (g_miniGCHandleStore == nullptr || !g_miniGCHandleStore->ContainsHandle(handle))
		return nullptr;
	Object** handleObject = (Object**)handle;
	return (Object*)MiniGCCompareExchangePointer(
		reinterpret_cast<void* volatile*>(handleObject), object, comparandObject);
}

HandleType MiniGCHandleManager::HandleFetchType(OBJECTHANDLE handle)
{
	return g_miniGCHandleStore == nullptr ? HNDTYPE_WEAK_SHORT : g_miniGCHandleStore->GetType(handle);
}

void MiniGCHandleManager::TraceRefCountedHandles(HANDLESCANPROC callback, uintptr_t param1, uintptr_t param2)
{
}

void MiniGCHandleManager::ScanHandles(promote_func* pf, ScanContext* sc)
{
	if (g_miniGCHandleStore != nullptr)
		g_miniGCHandleStore->ScanHandles(pf, sc);
}
