#include "pch.h"
#include "MiniGCHeap.h"
#include "MiniGCHeapConstants.h"
#include <iostream>
#include <cassert>
#include <cstddef>

#define GC_MARKED (size_t)0x1
const int GrowthSize = SEGMENT_SIZE;
int segmentsCount = 0;
uint8_t* segments[1024];

// ============================================================================
// GC <-> EE shared globals
//
// CoreCLR's write barrier and SOS look these up by name. They are declared
// extern "C" in coreclr gc.h, so the GC DLL must export them as unmangled
// C symbols with these exact names. (Declarations live in MiniGCHeap.h.)
// These definitions are not prefixed with `extern "C"` because, in C++, a
// global variable defined with C++ name-mangling would be incompatible with
// the extern "C" declaration in the header; the linker relies on the header
// declaration to give these symbols C linkage.
// ============================================================================
uint32_t  g_max_generation       = 2;            // soh_gen0..soh_gen2
uint8_t*  g_gc_lowest_address    = nullptr;      // filled in Initialize()
uint8_t*  g_gc_highest_address   = nullptr;      // filled in Initialize()
uint32_t* g_gc_card_table        = nullptr;      // [0 .. HEAP_ARENA_SIZE / CARD_SIZE)
uint32_t* g_gc_card_bundle_table = nullptr;      // [0 .. HEAP_ARENA_SIZE / CARD_BUNDLE_SIZE)

// ============================================================================
// Heap arena + card table state
//
// We reserve HEAP_ARENA_SIZE of virtual address up-front, then commit
// segments within it on demand. This gives a stable [lowest, highest] range
// and lets a single card table cover the entire arena.
// ============================================================================
static uint8_t* g_heap_arena = nullptr; // reserved VA, never moves
static uint8_t* g_arena_next = nullptr; // next byte to hand out for a new segment
static size_t   g_arena_committed = 0;  // total bytes currently committed

// Number of cards / bundles in the arena. Computed once when arena is set up.
static size_t g_card_table_count  = 0;
static size_t g_card_bundle_count = 0;

// Tracked frozen segments (the runtime pre-allocates these, e.g. the frozen
// object heap, and registers them with the GC). We record each range so that
// IsInFrozenSegment can answer membership queries and UpdateFrozenSegment can
// keep the committed/allocated limits current. Returning a null segment_handle
// from RegisterFrozenSegment is treated by CoreCLR as a heap-initialization
// failure (0x8007000E), so a non-null handle must be returned on success.
struct FrozenSegmentRecord
{
    uint8_t* base;
    uint8_t* allocated;
    uint8_t* committed;
    uint8_t* reserved;
};
FrozenSegmentRecord* frozenSegments[64];
int frozenSegmentsCount = 0;

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

	bool ContainsGCPointers()
	{
		return ContainsPointers();
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

#include "gcdesc.h"


bool MiniGCHeap::IsValidSegmentSize(size_t size)
{
	return size >= 64 * 1024 && size <= 1024ull * 1024 * 1024 && (size & (size - 1)) == 0;
}

bool MiniGCHeap::IsValidGen0MaxSize(size_t size)
{
	return size > 0 && size <= GrowthSize;
}

size_t MiniGCHeap::GetValidSegmentSize(bool large_seg)
{
	return GrowthSize;
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
	if (gcToCLR == nullptr || handleManager == nullptr)
		return E_FAIL;

	// Idempotency: Initialize can be called more than once across host
	// restarts. We only do the full arena + card table setup on the first
	// call so that a second Initialize doesn't double-allocate the arena.
	if (g_heap_arena != nullptr)
		return S_OK;

	// Reserve HEAP_ARENA_SIZE of contiguous virtual address. The whole arena
	// is committed at once for the prototype; finer-grained commit/release
	// can be added later by switching MiniGCAllocatePages-style helpers over
	// to operating on a sub-range of the arena.
	g_heap_arena = (uint8_t*)MiniGCAllocatePages(HEAP_ARENA_SIZE);
	if (g_heap_arena == nullptr)
		return E_OUTOFMEMORY;
	g_arena_next = g_heap_arena;
	g_arena_committed = HEAP_ARENA_SIZE;

	g_card_table_count  = HEAP_ARENA_SIZE / CARD_SIZE;
	g_card_bundle_count = HEAP_ARENA_SIZE / CARD_BUNDLE_SIZE;

	// The runtime's write barrier reads the card table as uint32_t, so we
	// also allocate it as uint32_t. Both VirtualAlloc and mmap return
	// zero-filled pages, so the table is "all cards clean" out of the box;
	// the runtime has never marked anything dirty at this point.
	g_gc_card_table = (uint32_t*)MiniGCAllocatePages(g_card_table_count * sizeof(uint32_t));
	if (g_gc_card_table == nullptr)
	{
		MiniGCFreePages(g_heap_arena, HEAP_ARENA_SIZE);
		g_heap_arena = nullptr;
		return E_OUTOFMEMORY;
	}

	g_gc_card_bundle_table = (uint32_t*)MiniGCAllocatePages(g_card_bundle_count * sizeof(uint32_t));
	if (g_gc_card_bundle_table == nullptr)
	{
		MiniGCFreePages(g_gc_card_table, g_card_table_count * sizeof(uint32_t));
		g_gc_card_table = nullptr;
		MiniGCFreePages(g_heap_arena, HEAP_ARENA_SIZE);
		g_heap_arena = nullptr;
		return E_OUTOFMEMORY;
	}

	// Publish the heap range to the EE so the write barrier can sanity-check
	// the bounds of pointers it sees. The first segment is allocated lazily
	// on the first Alloc, so we install a tight [arena, arena+0] range now
	// (empty range, highest < lowest) and update both bounds at the end of
	// the first Alloc that produces a real segment. Until then, the write
	// barrier will treat any interior pointer as out-of-range and skip it.
	g_gc_lowest_address  = g_heap_arena;
	g_gc_highest_address = g_heap_arena;

	// Hand the card table and the (initially empty) heap range to the EE.
	WriteBarrierParameters args{};
	args.operation             = WriteBarrierOp::Initialize;
	args.is_runtime_suspended  = false;
	args.requires_upper_bounds_check = false;
	args.card_table            = g_gc_card_table;
	args.card_bundle_table     = g_gc_card_bundle_table;
	args.lowest_address        = g_gc_lowest_address;
	args.highest_address       = g_gc_highest_address;
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
	if (acontext == nullptr || size == 0 || size > GrowthSize - 24)
		return nullptr;

	uint8_t* result = acontext->alloc_ptr;
	if (result != nullptr && acontext->alloc_limit != nullptr &&
		size <= static_cast<size_t>(acontext->alloc_limit - result))
	{
		acontext->alloc_ptr = result + size;
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
		static_cast<MiniGCHandleManager*>(handleManager)->ScanHandles(MiniGCHeap::MarkReachableRoot, &sc);
		gcToCLR->RestartEE(true);
		gcInProgress = false;
	}
	int beginGap = 24;

	// Carve the next segment out of the pre-reserved arena. The arena is
	// already committed in Initialize(), so this is just an offset bump.
	if (g_heap_arena == nullptr || g_arena_next + GrowthSize > g_heap_arena + HEAP_ARENA_SIZE)
		return nullptr;
	uint8_t* newPages = g_arena_next;
	g_arena_next += GrowthSize;

	if (!registerSegment(newPages))
		return nullptr;

	// On the first segment, publish the real [lowest, highest] range to the
	// EE so the write barrier can start checking pointer bounds correctly.
	// g_gc_lowest_address stays at g_heap_arena; we advance highest_address
	// to the first byte past the freshly-allocated segment.
	bool isFirstSegment = (g_gc_highest_address == g_heap_arena);
	if (isFirstSegment)
	{
		g_gc_highest_address = newPages + GrowthSize;
		WriteBarrierParameters args{};
		args.operation             = WriteBarrierOp::StompResize;
		args.is_runtime_suspended  = false;
		args.requires_upper_bounds_check = false;
		args.card_table            = g_gc_card_table;
		args.card_bundle_table     = g_gc_card_bundle_table;
		args.lowest_address        = g_gc_lowest_address;
		args.highest_address       = g_gc_highest_address;
		gcToCLR->StompWriteBarrier(&args);
	}

	uint8_t* allocationStart = newPages + beginGap;
	acontext->alloc_ptr = allocationStart + size;
	acontext->alloc_limit = newPages + GrowthSize;
	acontext->alloc_bytes += GrowthSize;
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


bool MiniGCHeap::registerSegment(uint8_t* new_pages)
{
	if (new_pages == nullptr || segmentsCount >= static_cast<int>(sizeof(segments) / sizeof(segments[0])))
		return false;
	segments[segmentsCount++] = new_pages;
	return true;
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
    if (pseginfo == nullptr || pseginfo->pvMem == nullptr)
        return segment_handle();
    if (frozenSegmentsCount >= static_cast<int>(sizeof(frozenSegments) / sizeof(frozenSegments[0])))
        return segment_handle();

    uint8_t* base = static_cast<uint8_t*>(pseginfo->pvMem);
    auto* rec = new FrozenSegmentRecord{
        base,
        base + pseginfo->ibAllocated,
        base + pseginfo->ibCommit,
        base + pseginfo->ibReserved
    };
    frozenSegments[frozenSegmentsCount++] = rec;
    return reinterpret_cast<segment_handle>(rec);
}

void MiniGCHeap::UnregisterFrozenSegment(segment_handle seg)
{
    if (seg == nullptr)
        return;
    auto* rec = reinterpret_cast<FrozenSegmentRecord*>(seg);
    for (int i = 0; i < frozenSegmentsCount; ++i)
    {
        if (frozenSegments[i] == rec)
        {
            frozenSegments[i] = frozenSegments[--frozenSegmentsCount];
            break;
        }
    }
    delete rec;
}

bool MiniGCHeap::IsInFrozenSegment(Object* object)
{
    uint8_t* p = reinterpret_cast<uint8_t*>(object);
    for (int i = 0; i < frozenSegmentsCount; ++i)
    {
        if (p >= frozenSegments[i]->base && p < frozenSegments[i]->reserved)
            return true;
    }
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
    if (seg == nullptr)
        return;
    auto* rec = reinterpret_cast<FrozenSegmentRecord*>(seg);
    if (allocated != nullptr)
        rec->allocated = allocated;
    if (committed != nullptr)
        rec->committed = committed;
}

int MiniGCHeap::RefreshMemoryLimit()
{
	return refresh_success;
}

enable_no_gc_region_callback_status MiniGCHeap::EnableNoGCRegionCallback(NoGCRegionCallbackFinalizerWorkItem* callback, uint64_t callback_threshold)
{
	return not_started;
}

FinalizerWorkItem* MiniGCHeap::GetExtraWorkForFinalization()
{
	return nullptr;
}

uint64_t MiniGCHeap::GetGenerationBudget(int generation)
{
	return generation == 0 ? GrowthSize : 0;
}

size_t MiniGCHeap::GetLOHThreshold()
{
	return 85000;
}

void MiniGCHeap::DiagWalkHeapWithACHandling(walk_fn fn, void* context, int gen_number, bool walk_large_object_heap_p)
{
	DiagWalkHeap(fn, context, gen_number, walk_large_object_heap_p);
}

void MiniGCHeap::NullBridgeObjectsWeakRefs(size_t length, void* unreachableObjectHandles)
{
}
