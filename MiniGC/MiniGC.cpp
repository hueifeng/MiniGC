#include "pch.h"
#include "MiniGC.h"
#include "MiniGCHeap.h"
#include "MiniGCHandleManager.h"
#include <stdio.h>
#include <exception>
#include <iostream>


extern "C" __declspec(dllexport) HRESULT GC_Initialize(
	/* In */  IGCToCLR * clrToGC,
	/* Out */ IGCHeap** gcHeap,
	/* Out */ IGCHandleManager * *gcHandleManager,
	/* Out */ GcDacVars * gcDacVars
)
{
	try
	{
		std::cout << "[MiniGC] GC_Initialize" << std::endl;
		IGCHandleManager* handleManager = new MiniGCHandleManager();
		IGCHeap* heap = new MiniGCHeap(clrToGC, handleManager);
		*gcHeap = heap;
		*gcHandleManager = handleManager;
		return S_OK;
	}
	catch (const std::exception& e)
	{
		printf(e.what());
	}
	return S_OK;
	

}

extern "C" __declspec(dllexport) void GC_VersionInfo(VersionInfo * info)
{
	info->MajorVersion = GC_INTERFACE_MAJOR_VERSION;
	info->MinorVersion = GC_INTERFACE_MINOR_VERSION;
	info->BuildVersion = 0;
	info->Name = "Mini GC";

}