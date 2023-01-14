#include "pch.h"
#include "MiniGCHeap.h"
#include <iostream>
#include <cassert>
#include "gcdesc.h"
#include <cstddef>

#define GC_MARKED (size_t)0x1
const int GrowthSize = 1 * 1024 * 1024;
int segmentsCount = 0;
uint8_t* segments[1024];

class ObjHeader
{
private:
#ifdef _WIN64
	DWORD m_alignpad;
#endif // _WIN64
	DWORD m_SyncBlockValue;
};

class Object
{
	MethodTable* m_pMethTab;
	uint32_t m_dwLength;

public:
	ObjHeader* GetHeader()
	{
		return ((ObjHeader*)this) - 1;
	}

	MethodTable* GetMethodTable() const
	{
		return ((MethodTable*)(((size_t)RawGetMethodTable()) & (~(GC_MARKED))));
	}

	MethodTable* RawGetMethodTable() const
	{
		return m_pMethTab;
	}

	void RawSetMethodTable(MethodTable* pMT)
	{
		m_pMethTab = pMT;
	}

	bool IsMarked()
	{
		return !!(((size_t)RawGetMethodTable()) & GC_MARKED);
	}

	void SetMarked()
	{
		RawSetMethodTable((MethodTable*)(((size_t)RawGetMethodTable()) | GC_MARKED));
	}

	void ClearMarked()
	{
		RawSetMethodTable(GetMethodTable());
	}

	uint32_t GetNumComponents()
	{
		//assert(GetMethodTable()->HasComponentSize());
		return m_dwLength;
	}
};
#define MTFlag_ContainsPointers     0x0100
#define MTFlag_HasCriticalFinalizer 0x0800
#define MTFlag_HasFinalizer         0x0010
#define MTFlag_IsArray              0x0008
#define MTFlag_Collectible          0x1000
#define MTFlag_HasComponentSize     0x8000


/*
 * The whole MethodTable is a part of EE-GC contract, we cannot change the layout or
 * masks used here.
 */
class MethodTable
{
private:
	uint16_t    m_componentSize;
	uint16_t    m_flags;
	uint32_t    m_baseSize;
	MethodTable* m_pRelatedType; // parent

	//const uint16_t MTFlag_ContainsPointers = 0x0100;
	////const uint16_t MTFlag_HasCriticalFinalizer = 0x0800;
	////const uint16_t MTFlag_HasFinalizer = 0x0010;
	////const uint16_t MTFlag_IsArray = 0x0008;
	//const uint16_t MTFlag_Collectible = 0x1000;
	//const uint16_t MTFlag_HasComponentSize = 0x8000;

public:
	uint32_t GetBaseSize()
	{
		return m_baseSize;
	}

	uint16_t GetComponentSize()
	{
		return m_componentSize;
	}

	static uint32_t GetTotalSize(Object* obj)
	{
		MethodTable* mT = obj->GetMethodTable();
		return (mT->GetBaseSize() +
			(mT->HasComponentSize() ? (obj->GetNumComponents() * mT->GetComponentSize()) : 0));
	}

	bool ContainsPointers()
	{
		return (m_flags & MTFlag_ContainsPointers) != 0;
	}

	bool HasComponentSize()
	{
		return (m_flags & MTFlag_HasComponentSize) != 0;
	}

	bool Collectible()
	{
		return (m_flags & MTFlag_Collectible) != 0;
	}

};


bool MiniGCHeap::IsValidSegmentSize(size_t size)
{
	std::cout << "IsValidSegmentSize" << std::endl;
	return false;
}

bool MiniGCHeap::IsValidGen0MaxSize(size_t size)
{
	std::cout << "IsValidGen0MaxSize" << std::endl;
	return false;
}

size_t MiniGCHeap::GetValidSegmentSize(bool large_seg)
{
	std::cout << "GetValidSegmentSize" << std::endl;
	return size_t();
}

void MiniGCHeap::SetReservedVMLimit(size_t vmlimit)
{
	std::cout << "SetReservedVMLimit" << std::endl;
}

void MiniGCHeap::WaitUntilConcurrentGCComplete()
{
	std::cout << "WaitUntilConcurrentGCComplete" << std::endl;
}

bool MiniGCHeap::IsConcurrentGCInProgress()
{
	std::cout << "IsConcurrentGCInProgress" << std::endl;
	return false;
}

void MiniGCHeap::TemporaryEnableConcurrentGC()
{
	std::cout << "TemporaryEnableConcurrentGC" << std::endl;
}

void MiniGCHeap::TemporaryDisableConcurrentGC()
{
}

bool MiniGCHeap::IsConcurrentGCEnabled()
{
	return false;
}

HRESULT MiniGCHeap::WaitUntilConcurrentGCCompleteAsync(int millisecondsTimeout)
{
	return NOERROR;
}

size_t MiniGCHeap::GetNumberOfFinalizable()
{
	return size_t();
}

Object* MiniGCHeap::GetNextFinalizable()
{
	return nullptr;
}

void MiniGCHeap::GetMemoryInfo(uint64_t* highMemLoadThresholdBytes, uint64_t* totalAvailableMemoryBytes, uint64_t* lastRecordedMemLoadBytes, uint64_t* lastRecordedHeapSizeBytes, uint64_t* lastRecordedFragmentationBytes, uint64_t* totalCommittedBytes, uint64_t* promotedBytes, uint64_t* pinnedObjectCount, uint64_t* finalizationPendingCount, uint64_t* index, uint32_t* generation, uint32_t* pauseTimePct, bool* isCompaction, bool* isConcurrent, uint64_t* genInfoRaw, uint64_t* pauseInfoRaw, int kind)
{
}

uint32_t MiniGCHeap::GetMemoryLoad()
{
	return 0;
}

int MiniGCHeap::GetGcLatencyMode()
{
	return 0;
}

int MiniGCHeap::SetGcLatencyMode(int newLatencyMode)
{
	return 0;
}

int MiniGCHeap::GetLOHCompactionMode()
{
	return 0;
}

void MiniGCHeap::SetLOHCompactionMode(int newLOHCompactionMode)
{
}

bool MiniGCHeap::RegisterForFullGCNotification(uint32_t gen2Percentage, uint32_t lohPercentage)
{
	return false;
}

bool MiniGCHeap::CancelFullGCNotification()
{
	return false;
}

int MiniGCHeap::WaitForFullGCApproach(int millisecondsTimeout)
{
	return 0;
}

int MiniGCHeap::WaitForFullGCComplete(int millisecondsTimeout)
{
	return 0;
}

unsigned MiniGCHeap::WhichGeneration(Object* obj)
{
	return 0;
}

int MiniGCHeap::CollectionCount(int generation, int get_bgc_fgc_coutn)
{
	return 0;
}

int MiniGCHeap::StartNoGCRegion(uint64_t totalSize, bool lohSizeKnown, uint64_t lohSize, bool disallowFullBlockingGC)
{
	return 0;
}

int MiniGCHeap::EndNoGCRegion()
{
	return 0;
}

size_t MiniGCHeap::GetTotalBytesInUse()
{
	return size_t();
}

uint64_t MiniGCHeap::GetTotalAllocatedBytes()
{
	return 0;
}

HRESULT MiniGCHeap::GarbageCollect(int generation, bool low_memory_p, int mode)
{
	return S_OK;
}

unsigned MiniGCHeap::GetMaxGeneration()
{
	return 0;
}

void MiniGCHeap::SetFinalizationRun(Object* obj)
{
}

bool MiniGCHeap::RegisterForFinalization(int gen, Object* obj)
{
	return false;
}

int MiniGCHeap::GetLastGCPercentTimeInGC()
{
	return 0;
}

size_t MiniGCHeap::GetLastGCGenerationSize(int gen)
{
	return size_t();
}

HRESULT MiniGCHeap::Initialize()
{
	std::cout << "MiniGCHeap Initialize" << std::endl;
	MethodTable* freeObjectMethodTable = gcToCLR->GetFreeObjectMethodTable();

	WriteBarrierParameters args = {};
	args.operation = WriteBarrierOp::Initialize;
	args.is_runtime_suspended = true;
	args.requires_upper_bounds_check = false;
	args.card_table = new uint32_t[1];
	args.lowest_address = reinterpret_cast<uint8_t*>(~0);;
	args.highest_address = reinterpret_cast<uint8_t*>(1);
	args.ephemeral_low = reinterpret_cast<uint8_t*>(~0);
	args.ephemeral_high = reinterpret_cast<uint8_t*>(1);
	gcToCLR->StompWriteBarrier(&args);

	return S_OK;
}

bool MiniGCHeap::IsPromoted(Object* object)
{
	return false;
}

bool MiniGCHeap::IsHeapPointer(void* object, bool small_heap_only)
{
	if (segmentsCount == 0)
		return false;
	for (int i = 0; i < segmentsCount; ++i)
	{
		uint8_t* address = (uint8_t*)object;
		if (address >= segments[i] &&
			address < segments[i] + GrowthSize)
			return true;
	}
	return false;
}

unsigned MiniGCHeap::GetCondemnedGeneration()
{
	return 0;
}

bool MiniGCHeap::IsGCInProgressHelper(bool bConsiderGCStart)
{
	return false;
}

unsigned MiniGCHeap::GetGcCount()
{
	return 0;
}

bool MiniGCHeap::IsThreadUsingAllocationContextHeap(gc_alloc_context* acontext, int thread_number)
{
	return acontext->alloc_limit != nullptr;
}

bool MiniGCHeap::IsEphemeral(Object* object)
{
	return false;
}

uint32_t MiniGCHeap::WaitUntilGCComplete(bool bConsiderGCStart)
{
	return 0;
}

void MiniGCHeap::FixAllocContext(gc_alloc_context* acontext, void* arg, void* heap)
{
}

size_t MiniGCHeap::GetCurrentObjSize()
{
	return size_t();
}

void MiniGCHeap::SetGCInProgress(bool fInProgress)
{
}

bool MiniGCHeap::RuntimeStructuresValid()
{
	return false;
}

void MiniGCHeap::SetSuspensionPending(bool fSuspensionPending)
{
}

void MiniGCHeap::SetYieldProcessorScalingFactor(float yieldProcessorScalingFactor)
{
}

void MiniGCHeap::Shutdown()
{
}

size_t MiniGCHeap::GetLastGCStartTime(int generation)
{
	return size_t();
}

size_t MiniGCHeap::GetLastGCDuration(int generation)
{
	return size_t();
}

size_t MiniGCHeap::GetNow()
{
	return size_t();
}

bool MiniGCHeap::gcInProgress = false;
IGCToCLR* MiniGCHeap::gcToCLR = nullptr;

Object* MiniGCHeap::Alloc(gc_alloc_context* acontext, size_t size, uint32_t flags)
{
	uint8_t* result = acontext->alloc_ptr;
	uint8_t* advance = result + size;
	if (advance <= acontext->alloc_limit)
	{
		acontext->alloc_ptr = advance;
		return (Object*)result;
	}
	if (acontext->alloc_limit != nullptr)
	{
		// Some allocation context filled, start GC
		gcInProgress = true;
		ScanContext sc;
		gcToCLR->SuspendEE(SUSPEND_FOR_GC);
		printf("GCLOG: Scan stack roots\n");
		gcToCLR->GcScanRoots(MiniGCHeap::MarkReachableRoot, 0, 0, &sc);
		printf("GCLOG: Scan handles roots\n");
		handleManager->ScanHandles(MiniGCHeap::MarkReachableRoot, &sc);
		gcToCLR->RestartEE(true);
	}
	int beginGap = 24;
	uint8_t* newPages = (uint8_t*)VirtualAlloc(NULL, GrowthSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	uint8_t* allocationStart = newPages + beginGap;
	acontext->alloc_ptr = allocationStart + size;
	acontext->alloc_limit = newPages + GrowthSize;
	acontext->alloc_bytes += GrowthSize;
	registerSegment(newPages);
	printf("GCLOG: Segment crated %p-%p\n", acontext->alloc_ptr, acontext->alloc_limit);
	// gcToCLR->EventSink()->FireGCCreateSegment_V1(newPages, growthSize, 0);
	return (Object*)(allocationStart);
}

void MiniGCHeap::MarkReachableRoot(Object** ppObject, ScanContext* sc, uint32_t flags)
{
	Object* obj = *ppObject;
	if (obj == nullptr)
		return;
	MethodTable* pMT = (*ppObject)->GetMethodTable();
	printf("GCLOG: Reachable root at %p MT %p (flags: %d)\n", obj, pMT, flags);
}

void MiniGCHeap::MarkObjectTransitively(Object* obj, ScanContext* sc, uint32_t flags)
{
	if (obj->IsMarked())
	{
		printf("GCLOG:    Mark - already marked\n");
		return;
	}
	obj->SetMarked();
	MethodTable* pMT = obj->RawGetMethodTable();
	if (pMT->Collectible())
	{
		printf("GCLOG:    Mark - collectible type\n");
		// TODO
		uint8_t* class_obj = gcToCLR->GetLoaderAllocatorObjectForGC(obj);
		uint8_t** poo = &class_obj;
		uint8_t* oo = *poo;
		// exp
	}
	if (pMT->ContainsPointers())
	{
		printf("GCLOG:    Mark - containing pointers type at %p MT %p\n", obj, pMT);
		int start_useful = 0;
		uint8_t* start = (uint8_t*)obj;
		uint32_t size = MethodTable::GetTotalSize(obj);
		CGCDesc* map = CGCDesc::GetCGCDescFromMT(pMT);
		CGCDescSeries* cur = map->GetHighestSeries();
		ptrdiff_t cnt = (ptrdiff_t)map->GetNumSeries();
		if (cnt >= 0)
		{
			CGCDescSeries* last = map->GetLowestSeries();
			uint8_t** parm = 0;
			do
			{
				assert(parm <= (uint8_t**)((obj)+cur->GetSeriesOffset()));
				parm = (uint8_t**)((obj)+cur->GetSeriesOffset());
				uint8_t** ppstop = (uint8_t**)((uint8_t*)parm + cur->GetSeriesSize() + (size));
				if (!start_useful || (uint8_t*)ppstop > (start))
				{
					if (start_useful && (uint8_t*)parm < (start)) parm = (uint8_t**)(start);
					while (parm < ppstop)
					{
						//exp
						parm++;
					}
				}
				cur--;
			} while (cur >= last);
		}
		else
		{
			/* Handle the repeating case - array of valuetypes */
		}
	}
}


void MiniGCHeap::registerSegment(uint8_t* new_pages)
{
	segments[segmentsCount++] = new_pages;
}


void MiniGCHeap::PublishObject(uint8_t* obj)
{
}

void MiniGCHeap::SetWaitForGCEvent()
{
}

void MiniGCHeap::ResetWaitForGCEvent()
{
}

bool MiniGCHeap::IsLargeObject(Object* pObj)
{
	return false;
}

void MiniGCHeap::ValidateObjectMember(Object* obj)
{
}

Object* MiniGCHeap::NextObj(Object* object)
{
	return nullptr;
}

Object* MiniGCHeap::GetContainingObject(void* pInteriorPtr, bool fCollectedGenOnly)
{
	return nullptr;
}

void MiniGCHeap::DiagWalkObject(Object* obj, walk_fn fn, void* context)
{
}

void MiniGCHeap::DiagWalkObject2(Object* obj, walk_fn2 fn, void* context)
{
}

void MiniGCHeap::DiagWalkHeap(walk_fn fn, void* context, int gen_number, bool walk_large_object_heap_p)
{
}

void MiniGCHeap::DiagWalkSurvivorsWithType(void* gc_context, record_surv_fn fn, void* diag_context, walk_surv_type type, int gen_number)
{
}

void MiniGCHeap::DiagWalkFinalizeQueue(void* gc_context, fq_walk_fn fn)
{
}

void MiniGCHeap::DiagScanFinalizeQueue(fq_scan_fn fn, ScanContext* context)
{
}

void MiniGCHeap::DiagScanHandles(handle_scan_fn fn, int gen_number, ScanContext* context)
{
}

void MiniGCHeap::DiagScanDependentHandles(handle_scan_fn fn, int gen_number, ScanContext* context)
{
}

void MiniGCHeap::DiagDescrGenerations(gen_walk_fn fn, void* context)
{
}

void MiniGCHeap::DiagTraceGCSegments()
{
}

void MiniGCHeap::DiagGetGCSettings(EtwGCSettingsInfo* settings)
{
}

bool MiniGCHeap::StressHeap(gc_alloc_context* acontext)
{
	return false;
}

segment_handle MiniGCHeap::RegisterFrozenSegment(segment_info* pseginfo)
{
	return segment_handle();
}

void MiniGCHeap::UnregisterFrozenSegment(segment_handle seg)
{
}

bool MiniGCHeap::IsInFrozenSegment(Object* object)
{
	return false;
}

void MiniGCHeap::ControlEvents(GCEventKeyword keyword, GCEventLevel level)
{
}

void MiniGCHeap::ControlPrivateEvents(GCEventKeyword keyword, GCEventLevel level)
{
}

unsigned int MiniGCHeap::GetGenerationWithRange(Object* object, uint8_t** ppStart, uint8_t** ppAllocated, uint8_t** ppReserved)
{
	return 0;
}

int64_t MiniGCHeap::GetTotalPauseDuration()
{
	return 0;
}

void MiniGCHeap::EnumerateConfigurationValues(void* context, ConfigurationValueFunc configurationValueFunc)
{
}

void MiniGCHeap::UpdateFrozenSegment(segment_handle seg, uint8_t* allocated, uint8_t* committed)
{
}


