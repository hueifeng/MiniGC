#pragma once
#include "MiniGCPlatform.h"
#include "debugmacros.h"
#include "gcenv.base.h"
#include "gcinterface.h"

class MiniGCHandleStore : public IGCHandleStore
{
public:
	// 通过 IGCHandleStore 继承
	virtual void Uproot();
	virtual bool ContainsHandle(OBJECTHANDLE handle);
	virtual OBJECTHANDLE CreateHandleOfType(Object* object, HandleType type);
	virtual OBJECTHANDLE CreateHandleOfType(Object* object, HandleType type, int heapToAffinitizeTo);
	virtual OBJECTHANDLE CreateHandleWithExtraInfo(Object* object, HandleType type, void* pExtraInfo);
	virtual OBJECTHANDLE CreateDependentHandle(Object* primary, Object* secondary);

	void ScanHandles(promote_func* pf, ScanContext* sc);
	void DestroyHandle(OBJECTHANDLE handle);
	Object* GetObject(OBJECTHANDLE handle) const;
	Object* GetDependentSecondary(OBJECTHANDLE handle) const;
	void SetDependentSecondary(OBJECTHANDLE handle, Object* object);
	HandleType GetType(OBJECTHANDLE handle) const;
	void SetExtraInfo(OBJECTHANDLE handle, void* extraInfo);
	void* GetExtraInfo(OBJECTHANDLE handle) const;
};
