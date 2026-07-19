#pragma once

// ============================================================================
// Heap address space, card table, and card bundle layout constants.
//
// These constants are the GC <-> EE contract for the write barrier:
//   * The JIT-emitted write barrier shifts an interior pointer right by
//     CARD_SIZE_SHIFT to compute a card index, then dirties that card.
//   * The GC, when scanning dirty cards, must walk the same mapping.
//
// If you change any of these values, the runtime's write barrier code will
// silently mis-compute card indices and the heap will be corrupted (or every
// reference write will trap). They must stay in lockstep with CoreCLR.
//
// For .NET 8/9/10 the CoreCLR GC hard-codes card_size = 256 bytes and
// card_bundle_size = 2048 bytes (= 8 cards per bundle, card_bundle_shift=11).
// See dotnet/runtime src/coreclr/gc/gc.h and gcpriv.h.
// ============================================================================

#include <stddef.h>
#include <stdint.h>

// One card covers 256 bytes of the heap. card_table[idx] is dirty/clean
// state for the heap range [idx*256, (idx+1)*256).
static constexpr size_t CARD_SIZE = 256;
static constexpr unsigned CARD_SIZE_SHIFT = 8;   // 1 << 8 == 256
static constexpr size_t CARD_MASK = CARD_SIZE - 1;

// A card bundle groups 8 cards = 2KB. The runtime uses bundles as the
// coarseness for background GC dirty-card scanning.
static constexpr size_t CARD_BUNDLE_SIZE = 2048;
static constexpr unsigned CARD_BUNDLE_SHIFT = 11; // 1 << 11 == 2048

// Reserved virtual address range for the GC heap. We reserve this up-front
// so that the card table can be a single contiguous array covering the entire
// arena, and so that lowest/highest addresses remain stable as we commit more
// segments. 128 MB matches the default SOH reservation CoreCLR uses for
// workstation GC on a small heap.
static constexpr size_t HEAP_ARENA_SIZE = 128ull * 1024 * 1024;

// Default segment size, must be a power of two, multiple of CARD_SIZE.
static constexpr size_t SEGMENT_SIZE = 1024 * 1024; // 1 MB
