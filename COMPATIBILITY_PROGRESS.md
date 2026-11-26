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
| E2-10 | partial | 1/5 | 4 | 0 | 20.0% |
| E2-11 | open | 0/5 | 0 | 5 | 0.0% |

A percentage is shown only for a finite checklist-case denominator. Source enumeration counts are reported separately and are not folded into these percentages.

## Acceptance case status

- Passing/complete: **3**; partial: **13**; pending: **38**; failing: **0**.
- Required evidence classes mapped: **54/54**; D: 42, S: 15, P: 11, DP: 49, I: 10 cases (categories can overlap; required evidence is not a passing result).
- Acceptance cases missing or invalid required evidence classes: **0**; IDs: none.
- Acceptance cases without an evidence classification: **38**; IDs: E2-01-AC001, E2-01-AC003, E2-01-AC004, E2-01-AC005, E2-03-AC001, E2-03-AC002, E2-03-AC003, E2-03-AC004, E2-04-AC001, E2-04-AC002, E2-04-AC003, E2-04-AC004, E2-04-AC005, E2-05-AC001, E2-05-AC002, E2-05-AC003, E2-05-AC004, E2-05-AC005, E2-06-AC001, E2-06-AC002, E2-06-AC003, E2-06-AC004, E2-06-AC005, E2-07-AC001, E2-07-AC002, E2-07-AC003, E2-07-AC004, E2-07-AC005, E2-08-AC001, E2-08-AC002, E2-08-AC004, E2-08-AC005, E2-09-AC001, E2-11-AC001, E2-11-AC002, E2-11-AC003, E2-11-AC004, E2-11-AC005.
- Unclassified source inventory entries: **1464**; explicitly excluded adapter/presentation entries: **10**.
- Evidence totals remain separate: D counts compiled-Pascal sessions; DP counts configured direct comparisons; S counts source-derived model rows; P counts recorded production-only regressions; I counts only completed full integrated fixtures.

## Evidence inventory

- Direct compiled-Pascal probe sessions in the last completed full suite: **124** (54 standard programs; 70 standalone sessions).
- Direct Pascal-to-production differential invocations in the last completed full suite: **53/53** passed; failed: **0**; last complete result: **53/53 configured Pascal-to-production differential invocations passed, including F09; zero unexplained state, cell-write, sound, .BRD, or stable-serialization differences in F09. F10 remains pending. The F05/F08 repeated identical-value redraw cadence is separately classified**.
- Direct `.HI` Pascal-to-shared-engine byte comparisons: **16/16** passed (1 save, 8 load/reset, 7 insertion cases); source-model rows: **256**.
- Direct Pascal `.BRD` `BoardClose` record: **289/289** payload bytes matched through shared LibZoo import/export; external file: 291 bytes; SHA-256 `e97bfa7b3c33355f54bd04e8574bf5eb3e4d14ad834918d021e590ad04a7dddd`.
- Direct Pascal `BoardClose`/`WorldSave` differential: **25928/25928** full `.ZZT` bytes matched production serialization; SHA-256 `5ca053d06dbc8673c93c02e65c97bdf4692995bd1642d6341cc1b8ed24f6f9fa`; unexplained mismatches: 0.
- Source-produced legacy-format samples in [`serialization_fixture_manifest.json`](tools/compatibility/serialization_fixture_manifest.json): **4/4** have complete byte comparisons; this establishes one valid sample per listed format, not the outstanding serialized-field and malformed-boundary matrix.
  - `.ZZT`: 25928/25928 bytes; SHA-256 `5ca053d06dbc8673c93c02e65c97bdf4692995bd1642d6341cc1b8ed24f6f9fa`; pass (DP).
  - `.SAV`: 26465/26465 bytes; SHA-256 `956ffd4165a10113f2b1c9fb9a396018bfa1eb59c566dffb612285f93eb41f37`; pass (DP).
  - `.BRD`: 289/289 bytes; SHA-256 `e97bfa7b3c33355f54bd04e8574bf5eb3e4d14ad834918d021e590ad04a7dddd`; pass (DP).
  - `.HI`: 1590/1590 bytes; SHA-256 `419fceab58ebf6880d90ba7d87b3584f08ac8af63564eab0d86276e7544da09c`; pass (DP).
- Source-derived expectation cases: **256**; these are separate from compiled-Pascal probes.
- Production-only regression cases: **1**; these remain separate from direct Pascal probes and differentials.
- Integrated replay fixtures: **9/10** complete from [`integrated_replay_fixture_manifest.json`](tools/compatibility/integrated_replay_fixture_manifest.json). Prototype comparisons: 41690 fields across 12 checkpoints; per-cycle checkpoints: present.
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

- Required fixtures: 10 (fixed by `e2-10-required-fixtures-v1`).
- Completed fixtures: 9/10; completed IDs: E2-10-F01, E2-10-F02, E2-10-F03, E2-10-F04, E2-10-F05, E2-10-F06, E2-10-F07, E2-10-F08, E2-10-F09; pending IDs: E2-10-F10.
- Prototype: INTEGPRB: six zero-speed cycles with L0/S0/T1/C1-C6/T2-save/T3-reload/F0 checkpoints; same-Pascal-file load and round-trip save; not one of the ten fixtures with 41690 total field comparisons across 12 checkpoints; per-checkpoint counts: variable: 3,473 at L0/S0/T2/T3/F0; 3,475 at T1 and each cycle; this is not counted as a complete E2-10 fixture.
- Logical sound-queue events in the prototype: 2 events / 20 event fields; checkpoint sound state: compared; overflow: False.
- Ordered playfield cell writes in the prototype: 9 events / 36 event fields; capture: matched; generated oracle sidecar SHA-256: 6f153a1c437a476912a973144f6ee3e8e483b6478dbc16b9039e58bad0d246fa.
- Generated `.SAV` comparison: 565/565 stable bytes matched; 396 classified positions (327 differing values); raw file size/hash: 961 bytes / `378a6aaae4ef3f8f040f720c006a8896c6a52d4b672b599d0367fb3ace704700`. Pascal save error result: no return value; failure-path outcome remains outstanding.
- Same-file reload/round-trip: Pascal WorldLoad=True, production zoo_world_load=0; 949/949 loaded-file bytes preserved exactly; 12 process pointer bytes classified; input and second-save hashes: `378a6aaae4ef3f8f040f720c006a8896c6a52d4b672b599d0367fb3ace704700` / `8b4d70ab16e3c730f592197a508b409d0bf0e6e244491bd979b738e75466b0e3`.
- Per-cycle checkpoint comparisons: implemented.
- State-transition checkpoints: implemented.
- First-divergence reporting: implemented.
- First-divergence diagnostic negative control: passed; First divergence at cycle 1, field 0 (ammo): Pascal=1, production=0; state summaries and surrounding field/input context were emitted. This instrumentation check receives no replay-fixture or compatibility-case completion credit.
- `E2-10-F01` DEMO start and ordinary traversal: **passed**; 12 checkpoints / 41472 state fields; 0 state differences; 0 sound events; 5 cell writes; 25241 stable save bytes compared with 0 differences.
- `E2-10-F02` walls, pushers, duplicators, and collision ordering: **passed**; 12 checkpoints / 42378 state fields; 0 state differences; 4 sound events; 16 cell writes; 643 stable save bytes compared with 0 differences.
- `E2-10-F03` combat, bullets, keys/doors, and pickups: **passed**; 12 checkpoints / 41122 state fields; 0 state differences; 7 sound events; 664 cell writes; 558 stable save bytes compared with 0 differences.
- `E2-10-F04` object stat order plus multiple interacting OOP scripts: **passed**; 12 checkpoints / 41904 state fields; 0 state differences; 0 sound events; 5 cell writes; 598 stable save bytes compared with 0 differences.
- `E2-10-F05` board edges, neighbor transitions, and passages: **passed**; 15 checkpoints / 51493 state fields; 0 state differences; 1 sound events; 6006 cell writes; 896 stable save bytes compared with 0 differences.
- `E2-10-F06` dark room, torch, Energizer, timers, and death: **passed**; 126 checkpoints / 438324 state fields; 0 state differences; 4 sound events; 3730 cell writes; 552 stable save bytes compared with 0 differences.
- `E2-10-F07` pause, zero/positive-speed scheduling, and delayed input: **passed**; 13 checkpoints / 44187 state fields; 0 state differences; 0 sound events; 4 cell writes; 461 stable save bytes compared with 0 differences.
- `E2-10-F08` save, restore, world reload, and board change: **passed**; 17 checkpoints / 58251 state fields; 0 state differences; 1 sound events; 6006 cell writes; 896 stable save bytes compared with 0 differences.
- `E2-10-F09` editor mutation, .BRD round-trip, and play of the edited board: **passed**; 12 checkpoints / 40788 state fields; 0 state differences; 0 sound events; 5 cell writes; 750 stable save bytes compared with 0 differences.
- `E2-10-F10` malformed-file recovery and high-score insertion path: **pending**; not recorded checkpoints / not recorded state fields; not recorded state differences; not recorded sound events; not recorded cell writes; not recorded stable save bytes compared with not recorded differences.
- DEMO start and ordinary traversal: 12 checkpoints; 41472 state-field comparisons total; passed (D+DP+I evidence).
- walls, pushers, duplicators, and collision ordering: 12 checkpoints; 42378 state-field comparisons total; passed (D+DP+I evidence).
- combat, bullets, keys/doors, and pickups: 12 checkpoints; 41122 state-field comparisons total; passed (D+DP+I evidence).
- object stat order plus multiple interacting OOP scripts: 12 checkpoints; 41904 state-field comparisons total; passed (D+DP+I evidence).
- board edges, neighbor transitions, and passages: 15 checkpoints; 51493 state-field comparisons total; passed (D+DP+I evidence).
- dark room, torch, Energizer, timers, and death: 126 checkpoints; 438324 state-field comparisons total; passed (D+DP+I evidence).
- pause, positive-speed scheduling, and delayed input: 13 checkpoints; 44187 state-field comparisons total; passed (D+DP+I evidence).
- save, restore, world reload, and board change: 17 checkpoints; 58251 state-field comparisons total; passed (D+DP+I evidence).
- editor mutation, .BRD round-trip, and play of the edited board: 12 checkpoints; 40788 state-field comparisons total; passed (D+DP+I evidence).
  - L0: 3456 fields; matched.
  - S0: 3456 fields; matched.
  - T1: 3456 fields; matched.
  - C1: 3456 fields; matched.
  - C2: 3456 fields; matched.
  - C3: 3456 fields; matched.
  - C4: 3456 fields; matched.
  - C5: 3456 fields; matched.
  - C6: 3456 fields; matched.
  - T2: 3456 fields; matched.
  - T3: 3456 fields; matched.
  - F0: 3456 fields; matched.
  - L0: 3498 fields; matched.
  - S0: 3498 fields; matched.
  - T1: 3498 fields; matched.
  - C1: 3498 fields; matched.
  - C2: 3552 fields; matched.
  - C3: 3552 fields; matched.
  - C4: 3552 fields; matched.
  - C5: 3552 fields; matched.
  - C6: 3552 fields; matched.
  - T2: 3542 fields; matched.
  - T3: 3542 fields; matched.
  - F0: 3542 fields; matched.
  - L0: 3416 fields; matched.
  - S0: 3416 fields; matched.
  - T1: 3455 fields; matched.
  - C1: 3455 fields; matched.
  - C2: 3424 fields; matched.
  - C3: 3424 fields; matched.
  - C4: 3424 fields; matched.
  - C5: 3430 fields; matched.
  - C6: 3430 fields; matched.
  - T2: 3416 fields; matched.
  - T3: 3416 fields; matched.
  - F0: 3416 fields; matched.
  - L0: 3492 fields; matched.
  - S0: 3492 fields; matched.
  - T1: 3492 fields; matched.
  - C1: 3492 fields; matched.
  - C2: 3492 fields; matched.
  - C3: 3492 fields; matched.
  - C4: 3492 fields; matched.
  - C5: 3492 fields; matched.
  - C6: 3492 fields; matched.
  - T2: 3492 fields; matched.
  - T3: 3492 fields; matched.
  - F0: 3492 fields; matched.
  - L0: 3397 fields; matched.
  - S0: 3397 fields; matched.
  - T1: 3397 fields; matched.
  - C1: 3397 fields; matched.
  - T2 (neighbor entry): 3414 fields; matched.
  - C2: 3414 fields; matched.
  - T3 (passage): 3463 fields; matched.
  - T4: 3463 fields; matched.
  - C3: 3463 fields; matched.
  - C4: 3463 fields; matched.
  - C5: 3463 fields; matched.
  - C6: 3463 fields; matched.
  - T2 (post-save): 3433 fields; matched.
  - T3 (post-load): 3433 fields; matched.
  - F0: 3433 fields; matched.
  - L0: 3399 fields; matched.
  - S0: 3399 fields; matched.
  - T1: 3422 fields; matched.
  - C1: 3422 fields; matched.
  - C2: 3422 fields; matched.
  - C3: 3528 fields; matched.
  - C4: 3528 fields; matched.
  - C5: 3528 fields; matched.
  - C6: 3528 fields; matched.
  - C7: 3528 fields; matched.
  - C8: 3528 fields; matched.
  - C9: 3528 fields; matched.
  - C10: 3528 fields; matched.
  - C11: 3528 fields; matched.
  - C12: 3528 fields; matched.
  - C13: 3528 fields; matched.
  - C14: 3528 fields; matched.
  - C15: 3528 fields; matched.
  - C16: 3528 fields; matched.
  - C17: 3528 fields; matched.
  - C18: 3528 fields; matched.
  - C19: 3528 fields; matched.
  - C20: 3528 fields; matched.
  - C21: 3528 fields; matched.
  - C22: 3528 fields; matched.
  - C23: 3528 fields; matched.
  - C24: 3528 fields; matched.
  - C25: 3528 fields; matched.
  - C26: 3528 fields; matched.
  - C27: 3528 fields; matched.
  - C28: 3528 fields; matched.
  - C29: 3528 fields; matched.
  - C30: 3528 fields; matched.
  - C31: 3528 fields; matched.
  - C32: 3528 fields; matched.
  - C33: 3528 fields; matched.
  - C34: 3528 fields; matched.
  - C35: 3528 fields; matched.
  - C36: 3528 fields; matched.
  - C37: 3528 fields; matched.
  - C38: 3528 fields; matched.
  - C39: 3528 fields; matched.
  - C40: 3528 fields; matched.
  - C41: 3528 fields; matched.
  - C42: 3528 fields; matched.
  - C43: 3528 fields; matched.
  - C44: 3528 fields; matched.
  - C45: 3528 fields; matched.
  - C46: 3528 fields; matched.
  - C47: 3528 fields; matched.
  - C48: 3528 fields; matched.
  - C49: 3528 fields; matched.
  - C50: 3528 fields; matched.
  - C51: 3528 fields; matched.
  - C52: 3528 fields; matched.
  - C53: 3528 fields; matched.
  - C54: 3528 fields; matched.
  - C55: 3528 fields; matched.
  - C56: 3528 fields; matched.
  - C57: 3528 fields; matched.
  - C58: 3528 fields; matched.
  - C59: 3528 fields; matched.
  - C60: 3528 fields; matched.
  - C61: 3528 fields; matched.
  - C62: 3528 fields; matched.
  - C63: 3528 fields; matched.
  - C64: 3528 fields; matched.
  - C65: 3528 fields; matched.
  - C66: 3528 fields; matched.
  - C67: 3430 fields; matched.
  - C68: 3430 fields; matched.
  - C69: 3430 fields; matched.
  - C70: 3430 fields; matched.
  - C71: 3430 fields; matched.
  - C72: 3430 fields; matched.
  - C73: 3430 fields; matched.
  - C74: 3430 fields; matched.
  - C75: 3430 fields; matched.
  - C76: 3430 fields; matched.
  - C77: 3430 fields; matched.
  - C78: 3430 fields; matched.
  - C79: 3430 fields; matched.
  - C80: 3430 fields; matched.
  - C81: 3430 fields; matched.
  - C82: 3430 fields; matched.
  - C83: 3430 fields; matched.
  - C84: 3430 fields; matched.
  - C85: 3430 fields; matched.
  - C86: 3430 fields; matched.
  - C87: 3430 fields; matched.
  - C88: 3430 fields; matched.
  - C89: 3430 fields; matched.
  - C90: 3430 fields; matched.
  - C91: 3430 fields; matched.
  - C92: 3430 fields; matched.
  - C93: 3430 fields; matched.
  - C94: 3430 fields; matched.
  - C95: 3430 fields; matched.
  - C96: 3430 fields; matched.
  - C97: 3430 fields; matched.
  - C98: 3430 fields; matched.
  - C99: 3430 fields; matched.
  - C100: 3430 fields; matched.
  - C101: 3430 fields; matched.
  - C102: 3430 fields; matched.
  - C103: 3430 fields; matched.
  - C104: 3430 fields; matched.
  - C105: 3430 fields; matched.
  - C106: 3430 fields; matched.
  - C107: 3430 fields; matched.
  - C108: 3430 fields; matched.
  - C109: 3430 fields; matched.
  - C110: 3430 fields; matched.
  - C111: 3430 fields; matched.
  - C112: 3430 fields; matched.
  - C113: 3430 fields; matched.
  - C114: 3430 fields; matched.
  - C115: 3430 fields; matched.
  - C116: 3430 fields; matched.
  - C117: 3430 fields; matched.
  - C118: 3430 fields; matched.
  - C119: 3430 fields; matched.
  - C120: 3430 fields; matched.
  - T2: 3416 fields; matched.
  - T3: 3416 fields; matched.
  - F0: 3416 fields; matched.
  - L0: 3399 fields; matched.
  - S0: 3399 fields; matched.
  - T1: 3399 fields; matched.
  - C1: 3399 fields; matched.
  - T2: 3399 fields; matched.
  - C2: 3399 fields; matched.
  - T3: 3399 fields; matched.
  - C3: 3399 fields; matched.
  - C4: 3399 fields; matched.
  - C5: 3399 fields; matched.
  - T2: 3399 fields; matched.
  - T3: 3399 fields; matched.
  - F0: 3399 fields; matched.
  - L0: 3399 fields; matched.
  - S0: 3399 fields; matched.
  - T1: 3399 fields; matched.
  - C1: 3399 fields; matched.
  - T2: 3416 fields; matched.
  - C2: 3416 fields; matched.
  - T3: 3465 fields; matched.
  - T4: 3465 fields; matched.
  - C3: 3465 fields; matched.
  - C4: 3465 fields; matched.
  - C5: 3465 fields; matched.
  - C6: 3465 fields; matched.
  - T2: 3435 fields; matched.
  - T3: 3399 fields; matched.
  - T4: 3435 fields; matched.
  - T5: 3399 fields; matched.
  - F0: 3399 fields; matched.
  - L0: 3399 fields; matched.
  - S0: 3399 fields; matched.
  - T1: 3399 fields; matched.
  - C1: 3399 fields; matched.
  - C2: 3399 fields; matched.
  - C3: 3399 fields; matched.
  - C4: 3399 fields; matched.
  - C5: 3399 fields; matched.
  - C6: 3399 fields; matched.
  - T2: 3399 fields; matched.
  - T3: 3399 fields; matched.
  - F0: 3399 fields; matched.

## Mismatches, limitations, and remaining work

- Known unexplained mismatches in the latest recorded run: **0**. This count does not imply complete coverage; unexplained mismatch triage is still a release gate.

### Mandatory remaining work

- E2-01: complete source-to-shared-engine routine mapping; centralize remaining editor mutations and normalize source inventory scope exclusions.
- E2-02: complete the frozen byte/error boundary matrix; stabilize or otherwise directly resolve short-header EditorTransferBoard state under controlled allocation history; compare that result against the shared import/error API.
- E2-03 through E2-08: close remaining lifecycle, element, OOP, allocator/alias, input/RNG, virtual-cell/text-window, and broader sound producer/timer acceptance cases.
- E2-09: complete editor board/property/stat/object/copy-paste mutations and their direct Pascal comparisons; high-score table byte semantics are shared and verified.
- E2-10: complete the final frozen integrated replay fixture F10 (malformed-file recovery and high-score insertion); retain zero unexplained differences.
- E2-11: after E2-01 through E2-10 close, complete the fresh staging run, repeat heap-sensitive cases, and publish the completion record.

### Non-blocking limitations and optional investigations

- Physical Android device testing, UI rendering, platform audio, and frontend adapters are outside Engine 2.0 scope.
- Allocator reuse, unread destination tails, BIOS time, and physical sound output follow the explicit environment-dependent compatibility contract in ENGINE_2.0_CHECKLIST.md.

## Known discrepancies and unresolved questions

- Short-payload .BRD editor import directly reaches BoardCreate and matches seven selected production board/player fields. Earlier short-header captures came from an invalid nine-character DOS filename and are discarded. Corrected SHORTHD attempts stall or fail with Turbo Pascal runtime error 203 at the freed-buffer BoardOpen; expected control flow and the production safe-reopen model are source-derived, not direct differentials.
- Player element metadata differs: Pascal mutates ElementDefs[4].Character while LibZoo uses a draw callback. Resulting virtual-cell behavior across all animation cases remains to be proven.
- Editor flood-fill and .HI table semantics are now shared and directly compared; placement/paste/stat mutation side effects and other Editor.PAS state changes remain outside the platform-neutral API.
- The synthetic integrated prototype and required fixtures F01-F08 pass their recorded state, sound, effective-cell, and save/load comparisons. F05 and F08 each counted 834 Pascal and 835 production identical-value redraws; F06 counted 169 on both sides and F07 counted 3 on both. The one-count F05/F08 difference is classified under the existing environment-dependent poll/render cadence rule. Required fixtures F09-F10 and Pascal WorldSave failure-path outcomes remain open; undefined/pointer positions follow the checklist contract.

## Latest verification record

- Latest full Pascal-runner attempt: exit code 0; Clean full runner passed: 124/124 direct sessions and 53/53 configured comparisons; integrated fixtures F01-F09 passed with zero unexplained F09 state, effective-cell, sound, .BRD, or stable-save differences. The F05/F08 same-value redraw cadence remains separately classified; F10 is pending; E2-10 is 9/10.
- Latest integrated Pascal differential `E2-10-F09`: exit 0; 40788 snapshot fields, 0 logical sound events, 5 ordered playfield cell writes, 750/750 generated-save stable bytes, and 1152/1152 loaded-file round-trip stable bytes matched.
- Toolchain: Borland Turbo Pascal 5.5 (TPC.EXE and TURBO.TPL; excluded from repository); DOSBox-X 2026.10.01; executable SHA-256 B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3; external tool, not redistributed.
- Source revision: LibZoo baseline `7fb3e89a8467e66a4b8cbc290c708b9fa5290886`; local LibZoo overlay `e5d2fe7`; Pascal source hashes in the manifest; root Git revision: 080cc69bf2abfa745ecc2ad37b5d27117dba8fc2.
- Last completed full Pascal suite exit code: 0; command recorded: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain build/pascal-compat-run/Z -DosboxExecutable <external DOSBox-X 2026.10.01 executable>; exit 0`.
- Run record: [`latest_verification.json`](tools/compatibility/latest_verification.json).

## Recent progress

- Generalized integrated replay snapshots to live stat counts and unique script owners; the clean Pascal suite passed 84 direct sessions and 43/43 configured differentials. The two-script, three-stat prototype matched 31,158 fields across nine checkpoints. E2-10 remains open at 0/10 fixtures.
- E2-02 latest clean rerun: 84 direct Pascal sessions and 43/43 configured comparisons passed. The short-payload editor transfer now matches outcome plus seven selected fields; the corrected short-header DOSBox-X attempt remains unresolved (stall/runtime error 203) and is excluded from pass counts. Offline Android debug/instrumentation APK builds passed for arm64-v8a and x86_64. A separate production-only short-header regression also passed: the safe-reopen model preserves seeded board fields and clears neighbors; it does not resolve direct Pascal parity.
- Pre-transition prototype checkpoint: the integrated Pascal/production replay matched 31,260 state fields, two sound events / 20 fields, and nine cell writes / 36 fields. The later T1 checkpoint addition is recorded in the latest result.
- Extended the integrated replay with ordered playfield cell writes. After the staged BRDXFER boundary fix, a clean full Pascal runner passed 84 direct sessions and 43/43 comparisons, with zero unexplained mismatches. The prototype remains outside E2-10 required fixtures, which are 0/10 complete.
- Added dynamic L/S/T/C/F replay checkpoints and normalized game-role comparison. The clean Pascal suite passed 84 direct sessions and 43/43 configured differentials; the prototype matched 34,744 fields across ten checkpoints, plus sound and cell-write traces. E2-10 remains at 0/10 required fixtures.
- Added generated .SAV differential capture to the E2-10 prototype. The clean run still passed 84 direct Pascal sessions and 43/43 configured differentials; ten state checkpoints, 34,744 fields, two sound events, nine cell writes, and 565 deterministic save bytes matched. Another 396 fresh-world undefined/pointer bytes are classified; the prototype remains 0/10 required fixtures and Pascal save error outcomes remain open.
- Extended INTEGPRB through a second state transition, same-file .SAV load, and round-trip save. The clean full run passed 84 direct Pascal sessions and 43/43 configured comparisons; 12 checkpoints/41,690 fields matched, WorldLoad=true matched zoo_world_load=0, 565 fresh-world stable bytes and 949 loaded-file bytes matched. Compiling and running the allocator-sensitive replay in separate fresh DOSBox-X sessions standardizes its allocation history. E2-10 remains partial at 0/10 fixtures.
- Added a direct Pascal BoardClose/WorldSave differential at the actual file boundary. The clean full runner passed 84 direct sessions and 44/44 configured comparison invocations; all 25,928 .ZZT bytes matched shared LibZoo output, and the 289-byte board record still round-trips through shared .BRD import/export. E2-02 AC001 remains partial; the four-format fixture manifest and field/error matrix remain open.
- Added a repeatable integrated first-divergence negative control to the clean Pascal runner. On 2026-11-25 the full runner passed 84 direct sessions and 44/44 comparisons; an intentional disposable C1 ammo mismatch was reported at cycle 1 with adjacent-field context, and the original Pascal capture SHA-256 remained unchanged. This verifies diagnostics only and leaves E2-10 at 0/10 fixtures.
- E2-10-F01 DEMO traversal completed in a clean 2026-11-25 run: 12 checkpoints / 41,472 state fields, five ordered cell writes, two right moves across board 1, and 25,241 stable bytes across save/load round-trip; 85 direct Pascal sessions and 45/45 configured comparisons passed; corpus is 1/10.
- E2-10-F02 walls/pushers/Duplicator replay passed in a clean 2026-11-25 run: 12 checkpoints / 42,378 state fields, four sound events, 16 cell writes, stable save comparisons, and zero unexplained differences; 86 direct Pascal sessions and 46/46 configured comparisons passed; the fixed corpus is 2/10.
- Added the frozen E2-10-F03 combat/key/door/pickup replay through the original compiled-Pascal GamePlayLoop and shared production zoo_tick; the normalized input hook is limited to its staged INPUT.PAS compile.
- Clean Pascal verification on ae4b501 passed 87 direct sessions and 47/47 configured differentials; F01-F03 now pass 12-checkpoint integrated comparisons with no unexplained differences.
- F03 matched 41,122 state fields, seven sound events / 186 fields, 664 cell writes / 2,656 fields, 558 stable fresh-save bytes, and 876 stable round-trip bytes; runtime verification passed after harness cleanup.
- F04 completed in the clean 2026-11-25 run at ca271c7: 88 direct Pascal sessions and 48/48 configured differentials passed; the stat-order fixture matched 41,904 fields across 12 checkpoints, five cell writes, 598 stable save bytes, and 962 round-trip bytes. F01-F04 are now 4/10; F05-F10 remain open.
- E2-10-F05 completed in the clean 2026-11-25 run at af5929bf67fc32c1600d90582a98792db4452bcf: 119 direct Pascal sessions and 49/49 configured differentials passed; the neighbor/passage replay matched 51,493 fields across 15 checkpoints, one sound event, 6,006 effective cell changes, 896 stable generated-save bytes, and 1,556 round-trip bytes. The 834/835 same-value redraw count is separately classified under the existing clock/poll cadence contract. The frozen corpus is 5/10; F06-F10 remain open.
- E2-10-F06 passed in a post-fix clean 2026-11-25 runner at 1c39252ba592d90760fdab1acc2a37c46fa0307f: 120 direct Pascal sessions and 50/50 configured differentials passed; the dark-room/torch/Energizer/time-limit death replay matched 126 checkpoints/438,324 fields, four sound events, 3,730 effective cell changes, and all stable save/load bytes. The first-divergence negative control passed in the same run. The frozen corpus is 6/10.
- E2-10-F07 passed in the clean 2026-11-25 runner at 2adbf0bb4021533f0477217003164749921d819f: 121 direct Pascal sessions and 51/51 configured comparisons passed; the positive-speed pause/delayed-input replay matched 13 checkpoints / 44,187 state fields, zero sound events, four effective cell changes, 461 stable fresh-save bytes, and 829 round-trip bytes. Same-value redraw counts matched 3/3. The frozen corpus is 7/10; F08-F10 remain open.
- E2-10-F08 passed in the clean 2026-11-25 runner at 6668babc88d5d53add6f0f1ceb4c1eac12f57dea: 122 direct Pascal sessions and 52/52 comparisons passed. The six-cycle save/restore/world-reload replay matched 17 checkpoints / 58,251 state fields, one sound event, 6,006 effective cell changes, 896 stable generated-save bytes, and 1,562 round-trip bytes. Pascal and production both loaded the .SAV and baseline .ZZT; the F05/F08 redraw cadence difference remains separately classified. The fixed corpus is 8/10; F09-F10 remain pending.
- E2-10-F09 passed in the clean 2026-11-25 runner committed as 080cc69bf2abfa745ecc2ad37b5d27117dba8fc2: corrected ledger count 124 direct Turbo Pascal sessions and 53/53 configured differentials; the real Pascal EditorLoop exported/imported .BRD before GamePlayLoop. The integrated comparison matched 12 checkpoints / 40,788 fields, five cell writes, zero sound events, all 324 .BRD bytes, 750 stable save bytes, and 1,152 round-trip bytes; F10 remains pending (9/10).

Generated by `python tools/compatibility/generate_progress.py`. Do not edit this report manually.
