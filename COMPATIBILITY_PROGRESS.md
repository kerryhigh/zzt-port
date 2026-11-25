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
| E2-02 | partial | 0/5 | 4 | 1 | 0.0% |
| E2-03 | open | 0/4 | 0 | 4 | 0.0% |
| E2-04 | open | 0/5 | 0 | 5 | 0.0% |
| E2-05 | open | 0/5 | 0 | 5 | 0.0% |
| E2-06 | open | 0/5 | 0 | 5 | 0.0% |
| E2-07 | open | 0/5 | 0 | 5 | 0.0% |
| E2-08 | partial | 0/5 | 1 | 4 | 0.0% |
| E2-09 | partial | 1/5 | 3 | 1 | 20.0% |
| E2-10 | partial | 0/5 | 2 | 3 | 0.0% |
| E2-11 | open | 0/5 | 0 | 5 | 0.0% |

A percentage is shown only for a finite checklist-case denominator. Source enumeration counts are reported separately and are not folded into these percentages.

## Acceptance case status

- Passing/complete: **2**; partial: **10**; pending: **42**; failing: **0**.
- Unclassified source inventory entries: **1465**; explicitly excluded adapter/presentation entries: **10**.
- Evidence totals remain separate: D counts compiled-Pascal sessions; DP counts configured direct comparisons; S counts source-derived model rows; P remains unenumerated; I counts only completed full integrated fixtures.

## Evidence inventory

- Direct compiled-Pascal probe sessions in the last completed full suite: **84** (54 standard programs; 30 standalone sessions).
- Direct Pascal-to-production differential invocations in the last completed full suite: **43/43** passed; failed: **0**; last complete result: **all configured comparisons passed in the clean runner; short-header editor-state differential remains pending**.
- Direct `.HI` Pascal-to-shared-engine byte comparisons: **16/16** passed (1 save, 8 load/reset, 7 insertion cases); source-model rows: **256**.
- Source-derived expectation cases: **256**; these are separate from compiled-Pascal probes.
- Production-only regression cases: **not enumerated; one targeted E2-02 short-header safe-reopen regression passed**; the finite case inventory is still being mapped.
- Integrated replay fixtures: **0/10** complete. Prototype comparisons: 31242 fields across 9 checkpoints; per-cycle checkpoints: present.
- Other suite results: serializer=passed; tools/serializer-compat/verify.ps1 exit 0; short-payload DP and short-header safe-reopen P cases passed; runtime=passed after integrated sound-state/event harness update; tools/runtime-compat/verify.ps1 exit 0; Android CP437/key bridge=passed; tools/android-compat/verify.ps1 exit 0; Android build=passed fresh offline on 2026-11-24: .\gradlew.bat --offline :app:assembleDebug :app:assembleDebugAndroidTest with GRADLE_USER_HOME=build\gradle-home; exit 0, 64 tasks (13 executed, 51 up-to-date), native engine built for arm64-v8a and x86_64; Windows CTest=passed after the E2-02 API change: CMake-generated zzt_core, zzt_windows, zzt_windows_smoke, zzt_renderer_test, and zzt_sound_pcm_test targets built with MSBuild 17.14.19 (Release x64, BuildProjectReferences=false); CTest 3/3 passed. The CMake ZERO_CHECK wrapper stalled, so explicit generated projects were built directly.

## Frozen source inventory

| Inventory | Cases | Classified with D/S/P/DP/I | Explicitly excluded | Unclassified |
|---|---:|---:|---:|---:|
| Pascal routines | 223 | 4 | 2 | 217 |
| Element callback routines | 58 | 0 | 0 | 58 |
| Pascal branch sites | 991 | 8 | 6 | 977 |
| OOP words and aliases | 58 | 0 | 0 | 58 |
| Serialized fields | 87 | 0 | 0 | 87 |
| RNG call sites | 29 | 0 | 0 | 29 |
| Editor routines requiring scope/mutation mapping | 27 | 3 | 2 | 22 |
| Input paths requiring core/adapter mapping | 11 | 0 | 0 | 11 |

Total unclassified source inventory entries: **1465**; explicitly excluded: **10**. Source locations, routine-to-production candidates, callback branches, parser words, serialized fields, RNG sites, editor procedures, and input routines are kept as explicit acceptance records; an empty test mapping is not treated as coverage.

## Per-cycle integrated replay

- Required fixtures: 10.
- Completed fixtures: 0.
- Prototype: INTEGPRB: six zero-speed cycles with loaded, loop-start, per-cycle, and final snapshots; two interacting objects; not one of the ten fixtures with 31242 total field comparisons across 9 checkpoints; per-checkpoint counts: variable: 3,470 loaded/start/final; 3,472 at each cycle; this is not counted as a complete E2-10 fixture.
- Logical sound-queue events in the prototype: 2 events / 20 event fields; checkpoint sound state: compared; overflow: False.
- Per-cycle checkpoint comparisons: implemented.
- State-transition checkpoints: outstanding.
- First-divergence reporting: implemented.
- INTEGPRB prototype: buffered movement, two boulder pushes, and two interacting #GIVE/#IDLE scripts: 9 checkpoints; 31242 state-field comparisons total; passed; 2 logical sound queue events and 20 event fields matched (DP evidence).
  - loaded initial state: 3470 fields; matched.
  - game-loop start: 3470 fields; matched.
  - cycle 1: 3472 fields; matched.
  - cycle 2: 3472 fields; matched.
  - cycle 3: 3472 fields; matched.
  - cycle 4: 3472 fields; matched.
  - cycle 5: 3472 fields; matched.
  - cycle 6: 3472 fields; matched.
  - final state: 3470 fields; matched.

## Mismatches, limitations, and remaining work

- Known unexplained mismatches in the latest recorded run: **0**. This count does not imply complete coverage; unexplained mismatch triage is still a release gate.

### Mandatory remaining work

- E2-01: complete source-to-shared-engine routine mapping; centralize remaining editor mutations and normalize source inventory scope exclusions.
- E2-02: complete the frozen byte/error boundary matrix; stabilize or otherwise directly resolve short-header EditorTransferBoard state under controlled allocation history; compare that result against the shared import/error API.
- E2-03 through E2-08: close remaining lifecycle, element, OOP, allocator/alias, input/RNG, virtual-cell/text-window, logical cell-write, and broader sound producer/timer acceptance cases.
- E2-09: complete editor board/property/stat/object/copy-paste mutations and their direct Pascal comparisons; high-score table byte semantics are shared and verified.
- E2-10: complete cell-write, state-transition, and generated-byte captures; create and pass the ten frozen integrated replay fixtures with zero unexplained differences.
- E2-11: after E2-01 through E2-10 close, complete the fresh staging run, repeat heap-sensitive cases, and publish the completion record.

### Non-blocking limitations and optional investigations

- Physical Android device testing, UI rendering, platform audio, and frontend adapters are outside Engine 2.0 scope.
- Allocator reuse, unread destination tails, BIOS time, and physical sound output follow the explicit environment-dependent compatibility contract in ENGINE_2.0_CHECKLIST.md.

## Known discrepancies and unresolved questions

- Short-payload .BRD editor import directly reaches BoardCreate and matches seven selected production board/player fields. Earlier short-header captures came from an invalid nine-character DOS filename and are discarded. Corrected SHORTHD attempts stall or fail with Turbo Pascal runtime error 203 at the freed-buffer BoardOpen; expected control flow and the production safe-reopen model are source-derived, not direct differentials.
- Player element metadata differs: Pascal mutates ElementDefs[4].Character while LibZoo uses a draw callback. Resulting virtual-cell behavior across all animation cases remains to be proven.
- Editor flood-fill and .HI table semantics are now shared and directly compared; placement/paste/stat mutation side effects and other Editor.PAS state changes remain outside the platform-neutral API.
- The integrated prototype now dynamically captures 3,470/3,472 fields per checkpoint, the complete script alias graph, unique script bytes, sound queue checkpoint state, and two logical SoundQueue calls (20 event fields), all matching in the current standalone differential. It still lacks logical cell-write streams, distinct state-transition checkpoints, generated file bytes, and the ten required fixtures.
- The latest full Pascal runner attempt compiled and ran INTEGPRB but timed out in the subsequent BRDXFER DOSBox-X session (30 seconds); a separate 60-second retry also timed out. This is an unresolved verification-run interruption, not a reported production/oracle differential.

## Latest verification record

- Latest full Pascal-runner attempt: exit code 1; partial attempt; not a complete clean Pascal suite. DOSBox-X did not exit from the BRDXFER session within the runner 30-second timeout. Retrying the same session separately with a 60-second timeout also timed out.
- The latest integrated Pascal differential is reported separately: exit 0; 31242 snapshot fields and 2 logical sound events matched.
- Toolchain: Borland Turbo Pascal 5.5 (TPC.EXE and TURBO.TPL; excluded from repository); DOSBox-X 2026.10.01; SHA-256 B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3; external tool, not redistributed.
- Source revision: LibZoo baseline `7fb3e89a8467e66a4b8cbc290c708b9fa5290886`; local LibZoo overlay `e5d2fe7`; Pascal source hashes in the manifest; root Git revision: a7642fd: test: capture integrated Pascal sound queue events (E2-10 partial).
- Last completed full Pascal suite exit code: 0; command recorded: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain <Turbo Pascal 5.5 directory> -DosboxExecutable <DOSBox-X 2026.10.01 executable>; exit 0`.
- Run record: [`latest_verification.json`](tools/compatibility/latest_verification.json).

## Recent progress

- Moved the original editor flood-fill queue into shared LibZoo while preserving its 256-slot, byte-counter traversal; all four direct Pascal maps pass through the shared implementation.
- Fresh clean Pascal verification passed 83 direct Turbo Pascal sessions and 42 configured differentials after the shared editor-fill change.
- Fresh offline Android debug and instrumentation APK builds passed for arm64-v8a and x86_64; the clean Windows x64 Release build and all three CTests passed.
- Moved .BRD codec into shared LibZoo API; Android JNI and the serializer harness call the same production source.
- Fresh clean Pascal suite passed 83 direct sessions and 42 configured differentials; the integrated prototype matched 30,789 fields across nine checkpoints, including RNG and fixture script state.
- Serializer, high-score, runtime, Android CP437/key, and Windows native CTest suites passed after the shared .BRD change.
- Frozen the initial E2-01 through E2-11 checklist and Pascal source inventory with SHA-256 hashes.
- Moved .HI initialization, load/reset, save, ranking, and insertion into the shared LibZoo API; Android retains file and CP437 entry adapters.
- Compared 16 complete .HI records against direct compiled-Pascal outputs with zero mismatches; 256 source-model rows remain separate S evidence.
- Rebuilt Android arm64-v8a/x86_64 and Windows x64 after the shared .HI refactor; Android builds and Windows CTest 3/3 passed.
- Generalized integrated replay snapshots to live stat counts and unique script owners; the clean Pascal suite passed 84 direct sessions and 43/43 configured differentials. The two-script, three-stat prototype matched 31,158 fields across nine checkpoints. E2-10 remains open at 0/10 fixtures.
- E2-02 latest clean rerun: 84 direct Pascal sessions and 43/43 configured comparisons passed. The short-payload editor transfer now matches outcome plus seven selected fields; the corrected short-header DOSBox-X attempt remains unresolved (stall/runtime error 203) and is excluded from pass counts. Offline Android debug/instrumentation APK builds passed for arm64-v8a and x86_64. A separate production-only short-header regression also passed: the safe-reopen model preserves seeded board fields and clears neighbors; it does not resolve direct Pascal parity.
- Extended the integrated Pascal/production prototype to compare sound queue state at each snapshot and capture SoundQueue request/result events. A direct integrated comparison matched 31,242 variable-size snapshot fields and two sound events (20 fields); runtime verification passed. The encompassing full Pascal runner currently stops at a BRDXFER DOSBox-X timeout, so this is partial evidence, not a new full-suite pass.

Generated by `python tools/compatibility/generate_progress.py`. Do not edit this report manually.
