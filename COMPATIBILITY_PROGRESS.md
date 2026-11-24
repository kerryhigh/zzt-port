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
| E2-09 | partial | 1/5 | 3 | 1 | 20.0% |
| E2-10 | partial | 0/5 | 1 | 4 | 0.0% |
| E2-11 | open | 0/5 | 0 | 5 | 0.0% |

A percentage is shown only for a finite checklist-case denominator. Source enumeration counts are reported separately and are not folded into these percentages.

## Acceptance case status

- Passing/complete: **2**; partial: **4**; pending: **48**; failing: **0**.
- Unclassified source inventory entries: **1468**; explicitly excluded adapter/presentation entries: **10**.
- Evidence totals remain separate: D counts compiled-Pascal sessions; DP counts configured direct comparisons; S counts source-derived model rows; P remains unenumerated; I counts only completed full integrated fixtures.

## Evidence inventory

- Direct compiled-Pascal probe sessions: **83** (54 standard programs; 29 standalone sessions).
- Direct Pascal-to-production differential comparisons: **42/42** passed; failed: **0**; run result: **all configured comparisons passed in the clean runner**.
- Direct `.HI` Pascal-to-shared-engine byte comparisons: **16/16** passed (1 save, 8 load/reset, 7 insertion cases); source-model rows: **256**.
- Source-derived expectation cases: **256**; these are separate from compiled-Pascal probes.
- Production-only regression cases: **not enumerated; production runtime harness passed**; the finite case inventory is still being mapped.
- Integrated replay fixtures: **0/10** complete. Prototype comparisons: 30591 fields across 9 checkpoints; per-cycle checkpoints: present.
- Other suite results: serializer=passed; tools/serializer-compat/verify.ps1 exit 0; runtime=passed in the clean Pascal run; tools/runtime-compat/verify.ps1 exit 0; shared LibZoo flood-fill and production Android placement adapter checks pass; Android CP437/key bridge=passed; tools/android-compat/verify.ps1 exit 0; Android build=passed fresh offline on 2026-11-24: .\gradlew.bat --offline :app:assembleDebug :app:assembleDebugAndroidTest; exit 0, 64 tasks (13 executed, 51 up-to-date), native engine built for arm64-v8a and x86_64; Windows CTest=passed fresh in build/windows-editor-refactor: MSVC 14.44.35207, Windows SDK 10.0.26100, CMake 3.31.6; full x64 Release ALL_BUILD succeeded; CTest 3/3 passed (80x25 engine/I-O smoke, sharp-bilinear, PCM timing).

## Frozen source inventory

| Inventory | Cases | Classified with D/S/P/DP/I | Explicitly excluded | Unclassified |
|---|---:|---:|---:|---:|
| Pascal routines | 223 | 3 | 2 | 218 |
| Element callback routines | 58 | 0 | 0 | 58 |
| Pascal branch sites | 991 | 6 | 6 | 979 |
| OOP words and aliases | 58 | 0 | 0 | 58 |
| Serialized fields | 87 | 0 | 0 | 87 |
| RNG call sites | 29 | 0 | 0 | 29 |
| Editor routines requiring scope/mutation mapping | 27 | 3 | 2 | 22 |
| Input paths requiring core/adapter mapping | 11 | 0 | 0 | 11 |

Total unclassified source inventory entries: **1468**; explicitly excluded: **10**. Source locations, routine-to-production candidates, callback branches, parser words, serialized fields, RNG sites, editor procedures, and input routines are kept as explicit acceptance records; an empty test mapping is not treated as coverage.

## Per-cycle integrated replay

- Required fixtures: 10.
- Completed fixtures: 0.
- Prototype: INTEGPRB: six zero-speed cycles with loaded, loop-start, per-cycle, and final snapshots; not one of the ten fixtures with 30591 total field comparisons across 9 checkpoints (3399 fields each); this is not counted as a complete E2-10 fixture.
- Per-cycle and state-transition checkpoint comparisons: implemented.
- First-divergence reporting: implemented.
- INTEGPRB prototype: buffered movement, two boulder pushes, and #GIVE/#IDLE: 9 checkpoints × 3399 fields = 30591 comparisons; passed (DP evidence).

## Mismatches, limitations, and remaining work

- Known unexplained mismatches in the latest recorded run: **0**. This count does not imply complete coverage; unexplained mismatch triage is still a release gate.

### Mandatory remaining work

- E2-01: complete source-to-shared-engine routine mapping; centralize remaining editor mutations and normalize source inventory scope exclusions.
- E2-02: complete the exhaustive byte/error boundary matrix and resolve truncated .BRD behavior.
- E2-03 through E2-08: close remaining lifecycle, element, OOP, allocator/alias, input/RNG, virtual-cell, text-window, and logical sound acceptance cases.
- E2-09: complete editor board/property/stat/object/copy-paste mutations and their direct Pascal comparisons; high-score table byte semantics are shared and verified.
- E2-10: extend snapshot fields to cover scripts/aliases, RNG, cell writes, sound events, transitions, and serialized bytes; complete the ten frozen replay fixtures.
- E2-11: perform the final clean staging run after E2-01 through E2-10 close; repeat heap-sensitive cases and publish the completion record.

### Non-blocking limitations and optional investigations

- Physical Android device testing, UI rendering, platform audio, and frontend adapters are outside Engine 2.0 scope.
- Allocator reuse, unread destination tails, BIOS time, and physical sound output follow the explicit environment-dependent compatibility contract in ENGINE_2.0_CHECKLIST.md.

## Known discrepancies and unresolved questions

- Truncated .BRD reads differ at the API boundary: Pascal records I/O error 100 and retains partial length/payload bytes; the shared production codec reports a library error and Android wrapper policy also resets/closes state. Exact engine result and reset semantics remain unresolved.
- Player element metadata differs: Pascal mutates ElementDefs[4].Character while LibZoo uses a draw callback. Resulting virtual-cell behavior across all animation cases remains to be proven.
- Editor flood-fill and .HI table semantics are now shared and directly compared; placement/paste/stat mutation side effects and other Editor.PAS state changes remain outside the platform-neutral API.
- The integrated prototype compares a limited field schema at loaded, loop-start, per-cycle, and final checkpoints. Script bytes/alias graphs, RNG, cell-write/sound-event streams, state transitions, and generated bytes are not yet included; it does not count toward the ten required fixtures.

## Latest verification record

- Toolchain: Borland Turbo Pascal 5.5 (TPC.EXE and TURBO.TPL; excluded from repository); DOSBox-X 2026.10.01; SHA-256 B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3; external tool, not redistributed.
- Source revision: LibZoo baseline `7fb3e89a8467e66a4b8cbc290c708b9fa5290886`; local LibZoo overlay `e5d2fe7`; Pascal source hashes in the manifest; root Git revision: ce9f561: shared LibZoo .HI table semantics, Pascal differential harness, and E2-09 inventory metadata.
- Pascal suite exit code: 0; command recorded: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain build/pascal-compat-run/Z -DosboxExecutable <external DOSBox-X 2026.10.01>; exit 0`.
- Run record: [`latest_verification.json`](tools/compatibility/latest_verification.json).

## Recent progress

- Moved the original editor flood-fill queue into shared LibZoo while preserving its 256-slot, byte-counter traversal; all four direct Pascal maps pass through the shared implementation.
- Fresh clean Pascal verification passed 83 direct Turbo Pascal sessions and 42 configured differentials after the shared editor-fill change.
- Fresh offline Android debug and instrumentation APK builds passed for arm64-v8a and x86_64; the clean Windows x64 Release build and all three CTests passed.
- Moved .BRD codec into shared LibZoo API; Android JNI and the serializer harness call the same production source.
- Fresh clean Pascal suite passed 83 direct sessions and 42 configured direct differentials; INTEGPRB matched 30,591 fields across nine checkpoints with first-divergence reporting.
- Serializer, high-score, runtime, Android CP437/key, and Windows native CTest suites passed after the shared .BRD change.
- Frozen the initial E2-01 through E2-11 checklist and Pascal source inventory with SHA-256 hashes.
- Moved .HI initialization, load/reset, save, ranking, and insertion into the shared LibZoo API; Android retains file and CP437 entry adapters.
- Compared 16 complete .HI records against direct compiled-Pascal outputs with zero mismatches; 256 source-model rows remain separate S evidence.
- Rebuilt Android arm64-v8a/x86_64 and Windows x64 after the shared .HI refactor; Android builds and Windows CTest 3/3 passed.

Generated by `python tools/compatibility/generate_progress.py`. Do not edit this report manually.
