using System;
using System.Diagnostics;
using System.Runtime;

var sw = Stopwatch.StartNew();

// gen0, gen1, gen2 generations. New objects start at gen0; surviving
// GCs promote them. Use this to confirm which bucket a fresh object
// lands in.
Console.WriteLine($"gen0={GC.GetGeneration(0)} gen1={GC.GetGeneration(new object())} gen2={GC.GetGeneration(new object[1000])}");

// Allocation pressure on gen0: each iteration allocates a 256-byte
// buffer and drops the reference. Once the gen0 budget is exceeded,
// a gen0 GC fires. The MiniGC trace will show [mgh] GarbageCollect.
const int iters = 200_000;
long bytes = 0;
for (int i = 0; i < iters; i++)
{
    var arr = new byte[256];
    bytes += arr.Length;
}
Console.WriteLine($"1. allocated ~{bytes / 1024 / 1024} MB, elapsed {sw.ElapsedMilliseconds} ms");
sw.Restart();

// Finalizer queue. After GC.Collect, unreachable Finalizable objects
// are queued on the finalizer thread; GC.WaitForPendingFinalizers
// blocks until they run. MiniGC's SetFinalizationRun is the entry point.
var finals = new Finalizable[1000];
for (int i = 0; i < finals.Length; i++) finals[i] = new Finalizable();
finals = null;
GC.Collect();
GC.WaitForPendingFinalizers();
Console.WriteLine($"2. finalizer thread drained, elapsed {sw.ElapsedMilliseconds} ms");
sw.Restart();

// Weak reference. Alive after GC because we haven't dropped the
// reference yet; collect again and the target is gone.
var big = new byte[8 * 1024 * 1024];
var weak = new WeakReference(big, trackResurrection: false);
GC.Collect();
Console.WriteLine($"3a. weak target alive while big is reachable? {weak.IsAlive}");
big = null;
GC.Collect();
Console.WriteLine($"3b. weak target alive after dropping big? {weak.IsAlive}, elapsed {sw.ElapsedMilliseconds} ms");

internal sealed class Finalizable
{
    private readonly byte[] _payload = new byte[128];
    ~Finalizable() { _ = _payload.Length; }
}
