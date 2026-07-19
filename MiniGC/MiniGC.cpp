#include "pch.h"
#include "MiniGC.h"
#include "MiniGCHeap.h"
#include "MiniGCHandleManager.h"
#include <stdio.h>
#include <string.h>
#include <exception>
#include <iostream>


MINIGC_EXPORT HRESULT GC_Initialize(
	/* In */  IGCToCLR * clrToGC,
	/* Out */ IGCHeap** gcHeap,
	/* Out */ IGCHandleManager * *gcHandleManager,
	/* Out */ GcDacVars * gcDacVars
)
{
	try
	{
		std::cout << "[MiniGC] GC_Initialize" << std::endl;
		if (gcHeap == nullptr || gcHandleManager == nullptr || clrToGC == nullptr)
			return E_INVALIDARG;
		*gcHeap = nullptr;
		*gcHandleManager = nullptr;
		if (gcDacVars != nullptr)
			memset(gcDacVars, 0, sizeof(GcDacVars));
		IGCHandleManager* handleManager = new MiniGCHandleManager();
		IGCHeap* heap = new MiniGCHeap(clrToGC, handleManager);
		*gcHeap = heap;
		*gcHandleManager = handleManager;
		return S_OK;
	}
	catch (const std::exception& e)
	{
		printf("[MiniGC] GC_Initialize failed: %s\n", e.what());
		if (gcHeap != nullptr)
			*gcHeap = nullptr;
		if (gcHandleManager != nullptr)
			*gcHandleManager = nullptr;
		return E_FAIL;
	}
	catch (...)
	{
		printf("[MiniGC] GC_Initialize failed with an unknown exception\n");
		if (gcHeap != nullptr)
			*gcHeap = nullptr;
		if (gcHandleManager != nullptr)
			*gcHandleManager = nullptr;
		return E_FAIL;
	}
	

}

MINIGC_EXPORT void GC_VersionInfo(VersionInfo * info)
{
	info->MajorVersion = GC_INTERFACE_MAJOR_VERSION;
	info->MinorVersion = GC_INTERFACE_MINOR_VERSION;
	info->BuildVersion = 0;
	info->Name = "Mini GC";

}
