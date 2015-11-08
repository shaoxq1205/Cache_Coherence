# Cache_Coherence
Simulate MSI, MESI, Dragon Cache Coherence protocols which are commonly used in current parallel computers.
Simulation records read/write misses, cache to cache transactions, memory traffic and other counters.

Files included:
1. Code: main.cc  cache.cc  cache.h
2. Trace files: canneal.04t.debug    canneal.04t.longTrace
3. Validation files: MSI/MESI/Dragon_debug.val    MSI/MESI/Dragon_long.val

Simulation Example (GCC):
>make
>./smp_cache 8192 8 64 4 0 canneal.04t.debug >! output.txt
>diff -iw output.txt MSI_debug.val

