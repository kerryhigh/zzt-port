# Engine 2.0 compatibility progress

**Milestone status:** in progress  
**Mandatory blockers closed:** 0/11  
**Latest verification date:** 2026-11-24  
**Frozen inventory:** revision `engine-2.0-initial-freeze` (2026-11-24)  
**Reference source hashes:** [`reference_source_hashes.sha256`](tools/compatibility/reference_source_hashes.sha256)

Counts below are generated from the frozen checklist inventory and the latest recorded verification run. Checklist completion markers do not close a release blocker until the blocker pass gate and clean rerun pass.

## Blocker status

| Blocker | Status | Complete / total acceptance cases | Partial | Pending | Checklist coverage |
|---|---:|---:|---:|---:|---:|
| E2-01 | partial | 1/5 | 0 | 4 | 20.0% |
| E2-02 | open | 0/5 | 0 | 5 | 0.0% |
| E2-03 | open | 0/4 | 0 | 4 | 0.0% |
| E2-04 | open | 0/5 | 0 | 5 | 0.0% |
| E2-05 | open | 0/5 | 0 | 5 | 0.0% |
| E2-06 | open | 0/5 | 0 | 5 | 0.0% |
| E2-07 | open | 0/5 | 0 | 5 | 0.0% |
| E2-08 | open | 0/5 | 0 | 5 | 0.0% |
| E2-09 | open | 0/5 | 0 | 5 | 0.0% |
| E2-10 | partial | 0/5 | 1 | 4 | 0.0% |
| E2-11 | open | 0/5 | 0 | 5 | 0.0% |

A percentage is shown only for a finite checklist-case denominator. Source enumeration counts are reported separately and are not folded into these percentages.

## Acceptance case status

- Passing/complete: **1**; partial: **1**; pending: **52**; failing: **0**.
- Unclassified source inventory entries: **1490**.
- Evidence totals remain separate: D counts compiled-Pascal sessions; DP counts configured direct comparisons; S counts source-derived model rows; P remains unenumerated; I counts only completed full integrated fixtures.

## Evidence inventory

- Direct compiled-Pascal probe sessions: **83** (54 standard programs; 29 standalone sessions).
- Direct Pascal-to-production differential comparisons in the current runner: **42** configured comparator invocations; run result: **passed**.
- Source-derived expectation cases: **256**; these are separate from compiled-Pascal probes.
- Production-only regression cases: **not enumerated; production runtime harness passed**; the finite case inventory is still being mapped.
- Integrated replay fixtures: **0/10** complete. Prototype final-snapshot fields: 3399; per-cycle checkpoints: not implemented.
- Other suite results: serializer=passed; tools/serializer-compat/verify.ps1 exit 0; runtime=passed; tools/runtime-compat/verify.ps1 exit 0; Android CP437/key bridge=passed; tools/android-compat/verify.ps1 exit 0; Windows CTest=passed after shared .BRD refactor; engine/I-O, sharp-bilinear renderer, and PCM timing.

## Frozen source inventory

| Inventory | Cases | Classified with D/S/P/DP/I | Unclassified |
|---|---:|---:|---:|
| Pascal routines | 223 | 0 | 223 |
| Element callback routines | 58 | 0 | 58 |
| Pascal branch sites | 991 | 0 | 991 |
| OOP words and aliases | 58 | 0 | 58 |
| Serialized fields | 87 | 0 | 87 |
| RNG call sites | 29 | 0 | 29 |
| Editor routines requiring scope/mutation mapping | 27 | 0 | 27 |
| Input paths requiring core/adapter mapping | 11 | 0 | 11 |

Total unclassified source inventory entries: **1490**. Source locations, routine-to-production candidates, callback branches, parser words, serialized fields, RNG sites, editor procedures, and input routines are kept as explicit acceptance records; an empty test mapping is not treated as coverage.

## Per-cycle integrated replay

- Required fixtures: 10.
- Completed fixtures: 0.
- Prototype: INTEGPRB: six deterministic zero-speed cycles, final-state snapshot only with 3399 final-snapshot fields; this is not counted as a complete E2-10 fixture.
- Per-cycle and state-transition checkpoint comparisons: outstanding.
- First-divergence reporting: outstanding.
- Current per-cycle results: none recorded.

## Mismatches, limitations, and remaining work

- Known unexplained mismatches in the latest recorded run: **0**. This count does not imply complete coverage; unexplained mismatch triage is still a release gate.

### Mandatory remaining work

- E2-01: complete source-to-shared-engine routine mapping; centralize editor mutations and .HI semantic operations.
- E2-02: complete the exhaustive byte/error boundary matrix and resolve truncated .BRD behavior.
- E2-03 through E2-09: close lifecycle, element, OOP, allocator/alias, input/RNG, virtual-cell/sound, editor/high-score acceptance inventories.
- E2-10: add initial/per-cycle/transition/final snapshots, first-divergence reporting, and all ten frozen replay fixtures.
- E2-11: perform the final clean staging run after E2-01 through E2-10 close; repeat heap-sensitive cases and publish the completion record.

### Non-blocking limitations and optional investigations

- Physical Android device testing, UI rendering, platform audio, and frontend adapters are outside Engine 2.0 scope.
- Allocator reuse, unread destination tails, BIOS time, and physical sound output follow the explicit environment-dependent compatibility contract in ENGINE_2.0_CHECKLIST.md.

## Known discrepancies and unresolved questions

- Truncated .BRD reads differ at the API boundary: Pascal records I/O error 100 and retains partial length/payload bytes; the shared production codec reports a library error and Android wrapper policy also resets/closes state. Exact engine result and reset semantics remain unresolved.
- Player element metadata differs: Pascal mutates ElementDefs[4].Character while LibZoo uses a draw callback. Resulting virtual-cell behavior across all animation cases remains to be proven.
- Editor board mutations and .HI load/rank/insert/save semantics are not yet all implemented through platform-neutral production APIs.
- The integrated gameplay differential compares a single final snapshot after six cycles; it has no initial, per-cycle, or transition checkpoints and does not count toward the ten required fixtures.

## Latest verification record

- Toolchain: Borland Turbo Pascal 5.5 (TPC.EXE and TURBO.TPL; excluded from repository); DOSBox-X; executable file version string unavailable, binary identified by SHA-256 in evidence note.
- Source revision: LibZoo baseline `7fb3e89a8467e66a4b8cbc290c708b9fa5290886`; local LibZoo overlay `e5d2fe7`; Pascal source hashes in the manifest; root Git revision: 4698e8f (source tree verified; reporting commit follows).
- Pascal suite exit code: 0; command recorded: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain <local Turbo Pascal 5.5> -DosboxExecutable <local DOSBox-X>`.
- Run record: [`latest_verification.json`](tools/compatibility/latest_verification.json).

## Recent progress

- Moved .BRD codec into shared LibZoo API; Android JNI and the serializer harness call the same production source.
- Fresh clean Pascal suite passed 83 direct sessions and the existing 3,399-field final-snapshot differential.
- Serializer, high-score, runtime, Android CP437/key, and Windows native CTest suites passed after the shared .BRD change.
- Frozen the initial E2-01 through E2-11 checklist and Pascal source inventory with SHA-256 hashes.

Generated by `python tools/compatibility/generate_progress.py`. Do not edit this report manually.
