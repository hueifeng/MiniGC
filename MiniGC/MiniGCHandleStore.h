#pragma once
#include "debugmacros.h"
#include "gcenv.base.h"
#include "gcinterface.h"

class MiniGCHandleStore : public IGCHandleStore
{
public:
	// Í¨¹ý IGCHandleStore ¼Ì³Ð
	virtual void Uproot();
	virtual bool ContainsHandle(OBJECTHANDLE handle);
	virtual OBJECTHANDLE CreateHandleOfType(Object* object, HandleType type);
	virtual OBJECTHANDLE CreateHandleOfType(Object* object, HandleType type, int heapToAffinitizeTo);
	virtual OBJECTHANDLE CreateHandleWithExtraInfo(Object* object, HandleType type, void* pExtraInfo);
	virtual OBJECTHANDLE CreateDependentHandle(Object* primary, Object* secondary);

	void ScanHandles(promote_func* pf, ScanContext* sc);
};