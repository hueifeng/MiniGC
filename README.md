# MiniGC

A standalone garbage collector for .NET 10. It plugs in via
`DOTNET_GCPath`. The point of this prototype is to learn the
CoreCLR ↔ GC boundary by exercising it on three platforms; it is
**not** a competitive GC, and mark/scan is stubbed.

## Status

| Subsystem | Status |
|---|---|
| Build on macOS, Linux, Windows | ✅ |
| `GC_Initialize` / `GC_VersionInfo` entry points | ✅ |
| Frozen-object-heap registration | ✅ |
| Heap arena + card table + `StompWriteBarrier` | ✅ |
| `IGCHeap::Alloc` (small-object heap) | ✅ |
| Finalization, weak/strong handles | ✅ |
| `GcDemo` sample (gen0 pressure, finalizers, weak refs) | ✅ |
| `MarkReachableRoot` (object relocator) | ⚠️ stub |
| `Object` / `MethodTable` field layout | ⚠️ stub |
| `GcDacVars` population | ⚠️ missing |

The current end of the road: CoreCLR loads MiniGC, allocates through
the arena, runs finalizers and weak-handle operations, then fails
when constructing a `System` exception because the object header
layout doesn't match what CoreCLR expects. The trace ends in a
`FailFast` inside `SR.InternalGetResourceString`.

## Platforms

| Platform | Build |
|---|---|
| macOS (Apple Silicon, Intel) | `cmake -S MiniGC -B MiniGC/build && cmake --build MiniGC/build` |
| Linux (glibc) | same CMake |
| Windows (x64) | `msbuild MiniGC\MiniGC.vcxproj /p:Configuration=Debug` |

GitHub Actions (`.github/workflows/build.yml`) builds the native DLL
plus the `GcDemo` sample on Ubuntu and macOS, on every push to `master`.

## Building and running

```bash
./launch-with-gcloader.sh
```

Builds MiniGC, builds `GcDemo`, runs it with `DOTNET_GCPath` set to
the just-built `libMiniGC.dylib` (macOS) or `libMiniGC.so` (Linux).

Without the script:

```bash
dotnet run --project GcDemo
```

runs `GcDemo` with CoreCLR's in-box GC for comparison.

On Windows, `msbuild MiniGC\MiniGC.vcxproj /p:Configuration=Debug`,
then `dotnet run --project GcDemo`.

## Card table contract

`CARD_SIZE` is 256, `CARD_BUNDLE_SIZE` is 2048 (8 cards per bundle).
These are baked into CoreCLR's write-barrier code generation; changing
them silently corrupts the heap. See `dotnet/runtime src/coreclr/gc/gc.h`
and the four symbols `g_gc_card_table`, `g_gc_card_bundle_table`,
`g_gc_lowest_address`, `g_gc_highest_address`.

## What's left

1. **Object header layout.** `Object` / `ObjHeader` / `MethodTable` in
   `MiniGCHeap.cpp` need to match what CoreCLR expects (no alignment
   pad before `MethodTable*`, sync block in the right place).
2. **Mark / relocate.** `MarkReachableRoot` is a printf stub.
3. **GcDacVars.** The 38 fields in `gcinterface.dacvars.def` aren't
   populated; SOS / DAC will deref nulls until they are.
