# Soft-key response: native CPU measurement

Engine baseline `2e0d45d851a8ecab5563746be4f83ddd29c69545` (0.1.5), compared
with the 0.1.6 bounded-key-travel correction. Both use the existing exact
pallet-opening cache, full Life/Touch, default quality, 48 kHz and 64-frame
blocks, Release `clang++ -O3 -std=c++17 -fno-builtin` on the owner's M1 Max.
Five alternating paired trials per case, each with 0.5 s warmup and 2 s
measured audio. Thread CPU and wall time are measured separately per block.

Median thread CPU, as a percentage of one core:

| Workload | 0.1.5 | Candidate | Relative change |
| --- | ---: | ---: | ---: |
| Idle | 0.96% | 0.97% | +0.6% |
| One held Init note | 2.08% | 2.11% | +1.2% |
| Master, 28 chambers held | 14.32% | 14.31% | −0.04% |
| Master, changing chords, up to 56 chambers | 14.71% | 14.66% | −0.3% |
| Same changing chords at velocity 8 | 20.89% | 19.10% | −8.6% |

Ordinary cases are effectively unchanged and their output hashes match.
The soft-chord case intentionally sounds different: its keys settle sooner,
so fewer samples need the changing-pallet calculation. This is not a
solver optimization and does not change oversampling or reed precision.

Candidate thread-CPU/wall deadline exceedances were zero in these measured
trials. The baseline ordinary chord case had 10 wall exceedances; the soft
chord case had 2 thread-CPU and 15 wall exceedances. All results are retained;
the difference is not a guarantee about scheduling, Reason's CPU meter or
longer sessions. Candidate soft-chord p99 block CPU was 408.1 microseconds,
maximum CPU 472.2 microseconds and maximum wall time 706.3 microseconds,
against a 1333.3 microsecond block interval. All outputs were finite.

The standard native `Respira.CpuBudget` suite also passes. Measurements and
probe source are in the owner's external soft-start evidence directory.
