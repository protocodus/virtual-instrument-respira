# Towards the 10 % block ceiling

The portfolio standard asks that no 256-frame block take more than 10 % of
one core at default settings. Respira 0.1.24 does not meet it. This is what
the measurements in
[`Docs/benchmarks/cpu-consolidation-2026-11-09.md`](../benchmarks/cpu-consolidation-2026-11-09.md)
say about getting there. Figures are development-machine figures on a plain
x86-64 build.

## Where it stands

| Demo | Worst block | Sounding cells then |
| --- | --- | --- |
| Clarinet Solo | 10 % | 1 (a bellows turn) |
| Bassoon Low, Violin Folk | 12 % | 9–10 |
| Paris Musette | 16 % | 12 |
| Tango Bandoneon, Bright Oboe | 19–21 % | 10–14 |
| Organ Bellows | 24–29 % | 16 |
| Master Tutti | 49–60 % | 44 |

A sounding cell (two tongues, a duct of up to 16 sections, a vent) costs
1.1–1.3 % of a core at the worst block. The ceiling therefore buys six or
seven sounding cells at the present step rate, and a five-rank register
plays five cells a key. **No numerically neutral change reaches the ceiling
for chords on the multi-rank registers**: every bit-exact rewrite measured,
including two cells per vector, gained nothing.

## Options, cheapest first

Each is audible or changes behaviour, so each is the owner's decision.

1. **Half the cells' step rate** (2× the host rate, with Oversampling still
   doubling it twice). Measured: × 0.554 instructions; worst blocks
   8–16 % for six demos, 27–34 % for Master Tutti. Cost: 1–6 dB more energy
   above 12 kHz on the brightest reeds, well under the music, and the top
   reed needs about 8 % more air to start. Waiting on the listening set
   `consolidation-2026-11-09/respira/half-rate`. Shipping it needs: the
   patch beside the set, `ReedTuning.h` regenerated at the new rate (or kept,
   with 0.1.24's step correction, which the candidate uses), the README's
   Oversampling description and Known gaps, the demos re-rendered by the main
   build, and `PresetsSitTogether` re-checked.
2. **The folds at a bellows turn.** They add 3 % of a core for 0.6–0.8 s and
   up to 6–8 % at the turn itself, which is what puts the single-reed demo at
   the ceiling. Letting a panel that only follows the pressure ramp count as
   settled would remove the plateau; it moves when late panels click, so it
   needs a listening set of its own. Not started.
3. **Fewer duct sections.** The duct is about a third of the cell's cost and
   its section count follows the chamber's length. Not measured here: a
   coarser duct moves the chamber's resonances, which the tuning table and
   the attack measurements depend on. It would be a model change, not a
   quick win.
4. **A cap on sounding cells**, or cells that share a chamber's air between
   ranks. Both change what a register is. Not proposed.

Options 1 and 2 together bring every demo but Master Tutti and Organ Bellows
under or near 10 %. Nothing measured here brings a 44-cell chord under it:
at half rate it needs about 0.6 % of a core per cell and has 44.

## Still to measure, not decisions

- **In Reason.** The Rack toolchain compiles with `-fno-builtin` and links
  its own maths: the manual hand's `pow` and the pallet's `cos` are 0.16 % of
  the instructions here, and their cost there is unknown. Measure them in a
  U45 before replacing them with `DspMath.h` forms, which would change the
  hand's law in its last bits.
- **Vector lowering.** Whether `llc` lowers a 2-lane square root to one
  instruction on both targets is unverified; the pair experiment says it
  would not pay on x86-64 either way. Single-precision lanes (four cells
  per vector) would change the sound and are doubtful: the tongue's
  stiffness term loses the tuning's last cents in `float`.

## The budget test

`Respira.CpuBudget` fails for every demo but Clarinet Solo, as it should
while the ceiling is not met. It was not loosened. If the owner accepts a
higher ceiling for the multi-rank registers, or adopts option 1, the
ceiling's owner (the "Prevent stale parameters and CPU overruns" work) sets
the ratchet from the worst-block column above, not from a loaded run.
