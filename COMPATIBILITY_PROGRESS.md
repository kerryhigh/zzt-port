# Engine 2.0 compatibility progress

**Milestone status:** in progress  
**Mandatory blockers closed:** 0/11  
**Latest verification date:** 2026-11-25  
**Frozen inventory:** revision `engine-2.0-initial-freeze` (2026-11-24)  
**Reference source hashes:** [`reference_source_hashes.sha256`](tools/compatibility/reference_source_hashes.sha256)

Counts below are generated from the frozen checklist inventory and the latest recorded verification run. Checklist completion markers do not close a release blocker until the blocker pass gate and clean rerun pass.

## Blocker status

| Blocker | Status | Complete / total acceptance cases | Partial | Pending | Checklist coverage |
|---|---:|---:|---:|---:|---:|
| E2-01 | partial | 1/5 | 0 | 4 | 20.0% |
| E2-02 | partial | 0/5 | 5 | 0 | 0.0% |
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

- Passing/complete: **2**; partial: **11**; pending: **41**; failing: **0**.
- Required evidence classes mapped: **54/54**; D: 42, S: 15, P: 11, DP: 49, I: 10 cases (categories can overlap; required evidence is not a passing result).
- Acceptance cases missing or invalid required evidence classes: **0**; IDs: none.
- Acceptance cases without an evidence classification: **40**; IDs: E2-01-AC001, E2-01-AC003, E2-01-AC004, E2-01-AC005, E2-03-AC001, E2-03-AC002, E2-03-AC003, E2-03-AC004, E2-04-AC001, E2-04-AC002, E2-04-AC003, E2-04-AC004, E2-04-AC005, E2-05-AC001, E2-05-AC002, E2-05-AC003, E2-05-AC004, E2-05-AC005, E2-06-AC001, E2-06-AC002, E2-06-AC003, E2-06-AC004, E2-06-AC005, E2-07-AC001, E2-07-AC002, E2-07-AC003, E2-07-AC004, E2-07-AC005, E2-08-AC001, E2-08-AC002, E2-08-AC004, E2-08-AC005, E2-09-AC001, E2-10-AC002, E2-10-AC005, E2-11-AC001, E2-11-AC002, E2-11-AC003, E2-11-AC004, E2-11-AC005.
- Unclassified source inventory entries: **1464**; explicitly excluded adapter/presentation entries: **10**.
- Evidence totals remain separate: D counts compiled-Pascal sessions; DP counts configured direct comparisons; S counts source-derived model rows; P remains unenumerated; I counts only completed full integrated fixtures.

## Evidence inventory

- Direct compiled-Pascal probe sessions in the last completed full suite: **84** (54 standard programs; 30 standalone sessions).
- Direct Pascal-to-production differential invocations in the last completed full suite: **44/44** passed; failed: **0**; last complete result: **44/44 configured comparison invocations passed in the latest clean full runner, including full-byte Pascal BoardClose/WorldSave versus shared LibZoo world serialization (25,928 bytes) and a separate 289-byte .BRD parser/export round-trip; zero unexplained mismatches in configured cases; short-header editor-state comparison remains pending**.
- Direct `.HI` Pascal-to-shared-engine byte comparisons: **16/16** passed (1 save, 8 load/reset, 7 insertion cases); source-model rows: **256**.
- Direct Pascal `.BRD` `BoardClose` record: **289/289** payload bytes matched through shared LibZoo import/export; external file: 291 bytes; SHA-256 `e97bfa7b3c33355f54bd04e8574bf5eb3e4d14ad834918d021e590ad04a7dddd`.
- Direct Pascal `BoardClose`/`WorldSave` differential: **25928/25928** full `.ZZT` bytes matched production serialization; SHA-256 `5ca053d06dbc8673c93c02e65c97bdf4692995bd1642d6341cc1b8ed24f6f9fa`; unexplained mismatches: 0.
- Source-produced legacy-format samples in [`serialization_fixture_manifest.json`](tools/compatibility/serialization_fixture_manifest.json): **4/4** have complete byte comparisons; this establishes one valid sample per listed format, not the outstanding serialized-field and malformed-boundary matrix.
  - `.ZZT`: 25928/25928 bytes; SHA-256 `5ca053d06dbc8673c93c02e65c97bdf4692995bd1642d6341cc1b8ed24f6f9fa`; pass (DP).
  - `.SAV`: 26465/26465 bytes; SHA-256 `956ffd4165a10113f2b1c9fb9a396018bfa1eb59c566dffb612285f93eb41f37`; pass (DP).
  - `.BRD`: 289/289 bytes; SHA-256 `e97bfa7b3c33355f54bd04e8574bf5eb3e4d14ad834918d021e590ad04a7dddd`; pass (DP).
  - `.HI`: 1590/1590 bytes; SHA-256 `419fceab58ebf6880d90ba7d87b3584f08ac8af63564eab0d86276e7544da09c`; pass (DP).
- Source-derived expectation cases: **256**; these are separate from compiled-Pascal probes.
- Production-only regression cases: **not enumerated; one targeted E2-02 short-header safe-reopen regression passed**; the finite case inventory is still being mapped.
- Integrated replay fixtures: **0/10** complete. Prototype comparisons: 41690 fields across 12 checkpoints; per-cycle checkpoints: present.
- Integrated generated save: **565/565** deterministic byte positions match; 396 positions are classified as uninitialized Pascal padding or process-specific pointers; differing classified values: 327; production result: 0.
- Integrated save reload: compiled Pascal WorldLoad=True; production zoo_world_load=0; round-trip save preserved 949/949 bytes; 12 process-pointer positions classified.
- Other suite results: serializer=passed; tools/serializer-compat/verify.ps1 exit 0; Pascal BoardClose/WorldSave versus shared LibZoo matched the full 25,928-byte .ZZT; the 289-byte BoardClose record also matched through shared .BRD import/export; short-payload DP and short-header safe-reopen P cases passed; runtime=passed; tools/runtime-compat/verify.ps1 rebuilt the updated integrated WebAssembly harness and exited 0; the clean full Pascal/production run also passed its integrated runtime and differential gates; Android CP437/key bridge=passed; tools/android-compat/verify.ps1 exit 0; Android build=passed fresh offline on 2026-11-24: .\gradlew.bat --offline :app:assembleDebug :app:assembleDebugAndroidTest with GRADLE_USER_HOME=build\gradle-home; exit 0, 64 tasks (13 executed, 51 up-to-date), native engine built for arm64-v8a and x86_64; Windows CTest=passed after the E2-02 API change: CMake-generated zzt_core, zzt_windows, zzt_windows_smoke, zzt_renderer_test, and zzt_sound_pcm_test targets built with MSBuild 17.14.19 (Release x64, BuildProjectReferences=false); CTest 3/3 passed. The CMake ZERO_CHECK wrapper stalled, so explicit generated projects were built directly.

## Frozen source inventory

| Inventory | Cases | Classified with D/S/P/DP/I | Explicitly excluded | Unclassified |
|---|---:|---:|---:|---:|
| Pascal routines | 223 | 5 | 2 | 216 |
| Element callback routines | 58 | 0 | 0 | 58 |
| Pascal branch sites | 991 | 8 | 6 | 977 |
| OOP words and aliases | 58 | 0 | 0 | 58 |
| Serialized fields | 87 | 0 | 0 | 87 |
| RNG call sites | 29 | 0 | 0 | 29 |
| Editor routines requiring scope/mutation mapping | 27 | 3 | 2 | 22 |
| Input paths requiring core/adapter mapping | 11 | 0 | 0 | 11 |

Total unclassified source inventory entries: **1464**; explicitly excluded: **10**. Source locations, routine-to-production candidates, callback branches, parser words, serialized fields, RNG sites, editor procedures, and input routines are kept as explicit acceptance records; an empty test mapping is not treated as coverage.

## Per-cycle integrated replay

- Required fixtures: 10.
- Completed fixtures: 0.
- Prototype: INTEGPRB: six zero-speed cycles with L0/S0/T1/C1-C6/T2-save/T3-reload/F0 checkpoints; same-Pascal-file load and round-trip save; not one of the ten fixtures with 41690 total field comparisons across 12 checkpoints; per-checkpoint counts: variable: 3,473 at L0/S0/T2/T3/F0; 3,475 at T1 and each cycle; this is not counted as a complete E2-10 fixture.
- Logical sound-queue events in the prototype: 2 events / 20 event fields; checkpoint sound state: compared; overflow: False.
- Ordered playfield cell writes in the prototype: 9 events / 36 event fields; capture: matched; generated oracle sidecar SHA-256: 6f153a1c437a476912a973144f6ee3e8e483b6478dbc16b9039e58bad0d246fa.
- Generated `.SAV` comparison: 565/565 stable bytes matched; 396 classified positions (327 differing values); raw file size/hash: 961 bytes / `378a6aaae4ef3f8f040f720c006a8896c6a52d4b672b599d0367fb3ace704700`. Pascal save error result: no return value; failure-path outcome remains outstanding.
- Same-file reload/round-trip: Pascal WorldLoad=True, production zoo_world_load=0; 949/949 loaded-file bytes preserved exactly; 12 process pointer bytes classified; input and second-save hashes: `378a6aaae4ef3f8f040f720c006a8896c6a52d4b672b599d0367fb3ace704700` / `8b4d70ab16e3c730f592197a508b409d0bf0e6e244491bd979b738e75466b0e3`.
- Per-cycle checkpoint comparisons: implemented.
- State-transition checkpoints: implemented.
- First-divergence reporting: implemented.
- First-divergence diagnostic negative control: passed; First divergence at cycle 1, field 0 (ammo): Pascal=1, production=0; context=[ammo:1->0, gems:0->0, health:100->100]. This instrumentation check receives no replay-fixture or compatibility-case completion credit.
- INTEGPRB prototype: buffered movement, two boulder pushes, and two interacting #GIVE/#IDLE scripts: 12 checkpoints; 41690 state-field comparisons total; passed; 2 logical sound queue events / 20 event fields and 9 playfield writes / 36 event fields matched; pause-to-play, save, and reload transitions matched; 565 fresh-world stable SAV bytes and 949 loaded-file non-pointer bytes matched (DP evidence).
  - loaded initial state: 3473 fields; matched.
  - game-loop start: 3473 fields; matched.
  - state transition 1 (pause-to-play): 3475 fields; matched.
  - cycle 1: 3475 fields; matched.
  - cycle 2: 3475 fields; matched.
  - cycle 3: 3475 fields; matched.
  - cycle 4: 3475 fields; matched.
  - cycle 5: 3475 fields; matched.
  - cycle 6: 3475 fields; matched.
  - state transition 2 (after first save): 3473 fields; matched.
  - state transition 3 (after loading the shared Pascal SAV): 3473 fields; matched.
  - final state after round-trip save: 3473 fields; matched.

## Mismatches, limitations, and remaining work

- Known unexplained mismatches in the latest recorded run: **0**. This count does not imply complete coverage; unexplained mismatch triage is still a release gate.

### Mandatory remaining work

- E2-01: complete source-to-shared-engine routine mapping; centralize remaining editor mutations and normalize source inventory scope exclusions.
- E2-02: complete the frozen byte/error boundary matrix; stabilize or otherwise directly resolve short-header EditorTransferBoard state under controlled allocation history; compare that result against the shared import/error API.
- E2-03 through E2-08: close remaining lifecycle, element, OOP, allocator/alias, input/RNG, virtual-cell/text-window, and broader sound producer/timer acceptance cases.
- E2-09: complete editor board/property/stat/object/copy-paste mutations and their direct Pascal comparisons; high-score table byte semantics are shared and verified.
- E2-10: extend named transition, generated-byte, return/error, sound, and cell-write verification across the frozen ten-fixture replay corpus; broaden same-file loaded-byte preservation and save/restore/load comparisons across varied worlds; create and pass all ten fixtures with zero unexplained differences.
- E2-11: after E2-01 through E2-10 close, complete the fresh staging run, repeat heap-sensitive cases, and publish the completion record.

### Non-blocking limitations and optional investigations

- Physical Android device testing, UI rendering, platform audio, and frontend adapters are outside Engine 2.0 scope.
- Allocator reuse, unread destination tails, BIOS time, and physical sound output follow the explicit environment-dependent compatibility contract in ENGINE_2.0_CHECKLIST.md.

## Known discrepancies and unresolved questions

- Short-payload .BRD editor import directly reaches BoardCreate and matches seven selected production board/player fields. Earlier short-header captures came from an invalid nine-character DOS filename and are discarded. Corrected SHORTHD attempts stall or fail with Turbo Pascal runtime error 203 at the freed-buffer BoardOpen; expected control flow and the production safe-reopen model are source-derived, not direct differentials.
- Player element metadata differs: Pascal mutates ElementDefs[4].Character while LibZoo uses a draw callback. Resulting virtual-cell behavior across all animation cases remains to be proven.
- Editor flood-fill and .HI table semantics are now shared and directly compared; placement/paste/stat mutation side effects and other Editor.PAS state changes remain outside the platform-neutral API.
- The integrated prototype matches 41,690 variable-size state fields across 12 checkpoints, including pause-to-play, post-save, and post-reload transitions; two sound events / 20 fields and nine ordered playfield writes / 36 fields also match. One generated 961-byte SAV matches at all 565 stable positions; 396 fresh-world undefined/pointer positions are classified (327 differ, 69 happen to match). Both sides load the exact Pascal-generated SAV (WorldLoad=true; zoo_world_load=0), then the round-trip save matches all 949 preserved non-pointer bytes while classifying the 12 process-specific stat pointer positions. Pascal WorldSave failure-path outcomes, other scenarios, and the ten required fixtures remain open.

## Latest verification record

- Latest full Pascal-runner attempt: exit code 0; complete clean runner; 84 direct sessions and 44/44 configured comparison invocations passed; Pascal BoardClose/WorldSave and shared LibZoo matched the complete 25,928-byte .ZZT, and the 289-byte BoardClose record round-trip passed; runtime checks passed; 12 integrated checkpoints / 41,690 fields, same-file Pascal WorldLoad=true and production zoo_world_load=0, 565 fresh-world stable save bytes and 949 loaded-file round-trip bytes matched; E2-02 and E2-10 remain open.
- The latest integrated Pascal differential is reported separately: exit 0; 41690 snapshot fields, 2 logical sound events, 9 ordered playfield cell writes, 565/565 first-save bytes, and 949/949 loaded-file round-trip bytes matched.
- Toolchain: Borland Turbo Pascal 5.5 (TPC.EXE and TURBO.TPL; excluded from repository); DOSBox-X 2026.10.01; executable SHA-256 072c1944635ccc545a816e128625e3405feab559021e4ef60b2d512f97f754c0; external tool, not redistributed.
- Source revision: LibZoo baseline `7fb3e89a8467e66a4b8cbc290c708b9fa5290886`; local LibZoo overlay `e5d2fe7`; Pascal source hashes in the manifest; root Git revision: c7dcbd22007f48c6e251cd8e0036497e43bfda5b: test: compare Pascal BoardClose WorldSave output (E2-02 partial).
- Last completed full Pascal suite exit code: 0; command recorded: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain build/pascal-compat-run/Z -DosboxExecutable <external DOSBox-X 2026.10.01 executable>; exit 0`.
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
- Pre-transition prototype checkpoint: the integrated Pascal/production replay matched 31,260 state fields, two sound events / 20 fields, and nine cell writes / 36 fields. The later T1 checkpoint addition is recorded in the latest result.
- Extended the integrated replay with ordered playfield cell writes. After the staged BRDXFER boundary fix, a clean full Pascal runner passed 84 direct sessions and 43/43 comparisons, with zero unexplained mismatches. The prototype remains outside E2-10 required fixtures, which are 0/10 complete.
- Added dynamic L/S/T/C/F replay checkpoints and normalized game-role comparison. The clean Pascal suite passed 84 direct sessions and 43/43 configured differentials; the prototype matched 34,744 fields across ten checkpoints, plus sound and cell-write traces. E2-10 remains at 0/10 required fixtures.
- Added generated .SAV differential capture to the E2-10 prototype. The clean run still passed 84 direct Pascal sessions and 43/43 configured differentials; ten state checkpoints, 34,744 fields, two sound events, nine cell writes, and 565 deterministic save bytes matched. Another 396 fresh-world undefined/pointer bytes are classified; the prototype remains 0/10 required fixtures and Pascal save error outcomes remain open.
- Extended INTEGPRB through a second state transition, same-file .SAV load, and round-trip save. The clean full run passed 84 direct Pascal sessions and 43/43 configured comparisons; 12 checkpoints/41,690 fields matched, WorldLoad=true matched zoo_world_load=0, 565 fresh-world stable bytes and 949 loaded-file bytes matched. Compiling and running the allocator-sensitive replay in separate fresh DOSBox-X sessions standardizes its allocation history. E2-10 remains partial at 0/10 fixtures.
- Added a direct Pascal BoardClose/WorldSave differential at the actual file boundary. The clean full runner passed 84 direct sessions and 44/44 configured comparison invocations; all 25,928 .ZZT bytes matched shared LibZoo output, and the 289-byte board record still round-trips through shared .BRD import/export. E2-02 AC001 remains partial; the four-format fixture manifest and field/error matrix remain open.

Generated by `python tools/compatibility/generate_progress.py`. Do not edit this report manually.
