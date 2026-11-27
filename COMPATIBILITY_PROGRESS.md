# Engine 2.0 compatibility progress

**Milestone status:** in progress  
**Mandatory blockers closed:** 1/11  
**Latest verification date:** 2026-11-27  
**Frozen inventory:** revision `engine-2.0-initial-freeze` (2026-11-24)  
**Reference source hashes:** [`reference_source_hashes.sha256`](tools/compatibility/reference_source_hashes.sha256)

Counts below are generated from the frozen checklist inventory and the latest recorded verification run. Checklist completion markers do not close a release blocker until the blocker pass gate and clean rerun pass.

## Blocker status

| Blocker | Status | Complete / total acceptance cases | Partial | Pending | Checklist coverage |
|---|---:|---:|---:|---:|---:|
| E2-01 | partial | 1/5 | 0 | 4 | 20.0% |
| E2-02 | partial | 0/5 | 5 | 0 | 0.0% |
| E2-03 | partial | 2/4 | 1 | 1 | 50.0% |
| E2-04 | open | 0/5 | 0 | 5 | 0.0% |
| E2-05 | open | 0/5 | 0 | 5 | 0.0% |
| E2-06 | open | 0/5 | 0 | 5 | 0.0% |
| E2-07 | open | 0/5 | 0 | 5 | 0.0% |
| E2-08 | partial | 0/5 | 1 | 4 | 0.0% |
| E2-09 | partial | 1/5 | 3 | 1 | 20.0% |
| E2-10 | closed | 5/5 | 0 | 0 | 100.0% |
| E2-11 | open | 0/5 | 0 | 5 | 0.0% |

A percentage is shown only for a finite checklist-case denominator. Source enumeration counts are reported separately and are not folded into these percentages.

## Acceptance case status

- Passing/complete: **9**; partial: **10**; pending: **35**; failing: **0**.
- Required evidence classes mapped: **54/54**; D: 42, S: 15, P: 11, DP: 49, I: 10 cases (categories can overlap; required evidence is not a passing result).
- Acceptance cases missing or invalid required evidence classes: **0**; IDs: none.
- Acceptance cases without an evidence classification: **35**; IDs: E2-01-AC001, E2-01-AC003, E2-01-AC004, E2-01-AC005, E2-03-AC004, E2-04-AC001, E2-04-AC002, E2-04-AC003, E2-04-AC004, E2-04-AC005, E2-05-AC001, E2-05-AC002, E2-05-AC003, E2-05-AC004, E2-05-AC005, E2-06-AC001, E2-06-AC002, E2-06-AC003, E2-06-AC004, E2-06-AC005, E2-07-AC001, E2-07-AC002, E2-07-AC003, E2-07-AC004, E2-07-AC005, E2-08-AC001, E2-08-AC002, E2-08-AC004, E2-08-AC005, E2-09-AC001, E2-11-AC001, E2-11-AC002, E2-11-AC003, E2-11-AC004, E2-11-AC005.
- Unclassified source inventory entries: **1464**; explicitly excluded adapter/presentation entries: **10**.
- Evidence totals remain separate: D counts compiled-Pascal sessions; DP counts configured direct comparisons; S counts source-derived model rows; P counts recorded production-only regressions; I counts only completed full integrated fixtures.

## Evidence inventory

- Direct compiled-Pascal probe sessions in the last completed full suite: **148** (54 standard programs; 94 standalone sessions).
- Direct Pascal-to-production differential invocations in the last completed full suite: **66/66** passed; failed: **0**; last complete result: **66/66 direct Pascal-to-production comparison invocations passed on source revision a21746dd55bd1ba680a5749491be6838c241fc88: 65 existing configured comparisons plus the signed .BRD boundary comparator, which checked five Pascal-generated values and all 289 bytes per record. Frozen F01-F10 matched at 238 checkpoints / 824,821 checkpoint fields / 169 logical cycles. F10 post-failed-WorldLoad state matched 3,385/3,385 comparable fields; private World.BoardLen[0] is excluded. No unexplained mismatch**.
- Direct `.HI` Pascal-to-shared-engine byte comparisons: **16/16** passed (1 save, 8 load/reset, 7 insertion cases); source-model rows: **256**.
- Direct Pascal `.BRD` `BoardClose` record: **289/289** payload bytes matched through shared LibZoo import/export; external file: 291 bytes; SHA-256 `e97bfa7b3c33355f54bd04e8574bf5eb3e4d14ad834918d021e590ad04a7dddd`.
- Direct Pascal `BoardClose`/`WorldSave` differential: **25928/25928** full `.ZZT` bytes matched production serialization; SHA-256 `5ca053d06dbc8673c93c02e65c97bdf4692995bd1642d6341cc1b8ed24f6f9fa`; unexplained mismatches: 0.
- Source-produced legacy-format samples in [`serialization_fixture_manifest.json`](tools/compatibility/serialization_fixture_manifest.json): **4/4** have complete byte comparisons; this establishes one valid sample per listed format, not the outstanding serialized-field and malformed-boundary matrix.
  - `.ZZT`: 25928/25928 bytes; SHA-256 `5ca053d06dbc8673c93c02e65c97bdf4692995bd1642d6341cc1b8ed24f6f9fa`; pass (DP).
  - `.SAV`: 26465/26465 bytes; SHA-256 `956ffd4165a10113f2b1c9fb9a396018bfa1eb59c566dffb612285f93eb41f37`; pass (DP).
  - `.BRD`: 289/289 bytes; SHA-256 `e97bfa7b3c33355f54bd04e8574bf5eb3e4d14ad834918d021e590ad04a7dddd`; pass (DP).
  - `.HI`: 1590/1590 bytes; SHA-256 `419fceab58ebf6880d90ba7d87b3584f08ac8af63564eab0d86276e7544da09c`; pass (DP).
- Source-derived expectation cases: **256**; these are separate from compiled-Pascal probes.
- Production-only regression cases: **2**; these remain separate from direct Pascal probes and differentials.
- Integrated replay fixtures: **10/10** complete from [`integrated_replay_fixture_manifest.json`](tools/compatibility/integrated_replay_fixture_manifest.json). Prototype comparisons: 41690 fields across 12 checkpoints; per-cycle checkpoints: present.
- Integrated generated save: **565/565** deterministic byte positions match; 396 positions are classified as uninitialized Pascal padding or process-specific pointers; differing classified values: 327; production result: 0.
- Integrated save reload: compiled Pascal WorldLoad=True; production zoo_world_load=0; round-trip save preserved 949/949 bytes; 12 process-pointer positions classified.
- Serializer suite: passed fresh after source revision e2bbf2d45f535c03b532b309ae6f2eaefd30d6d8; tools/serializer-compat/verify.ps1 exited 0. Exhaustive codec check covered all 65,536 signed words and 256 packed signed bytes; existing .ZZT/.SAV/.BRD fixtures, all 16 DEMO boards, malformed reads, RLE, stale strings, raw Boolean/reserved bytes, and stat aliasing also passed. Full per-field/error-boundary matrix remains open.
- Runtime compatibility suite: passed fresh after source revision e2bbf2d45f535c03b532b309ae6f2eaefd30d6d8; tools/runtime-compat/verify.ps1 exited 0.
- Android CP437/key bridge: passed fresh after source revision e2bbf2d45f535c03b532b309ae6f2eaefd30d6d8; powershell -NoProfile -ExecutionPolicy Bypass -File tools/android-compat/verify.ps1; exit 0; Android 60x25 raster and DOS keyboard mapping cover all 256 CP437 bytes, colors, blink phases, and Pascal monochrome rules.
- Android build: passed fresh after source revision e2bbf2d45f535c03b532b309ae6f2eaefd30d6d8; .\gradlew.bat --gradle-user-home build\gradle-home --no-daemon --offline :app:assembleDebug :app:assembleDebugAndroidTest; exit 0; 64 actionable tasks (7 executed, 57 up-to-date); CMake built arm64-v8a and x86_64 native engine libraries. No physical Android device test was performed.
- Windows build/CTest: passed fresh after source revision e2bbf2d45f535c03b532b309ae6f2eaefd30d6d8; powershell -NoProfile -ExecutionPolicy Bypass -File windows/build.ps1; exit 0; Release x64 targets built; CTest 3/3 passed: Win32 engine I/O/80x25, sharp-bilinear scaling, and Pascal drum timing.

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
- Completed fixtures: 10/10; completed IDs: E2-10-F01, E2-10-F02, E2-10-F03, E2-10-F04, E2-10-F05, E2-10-F06, E2-10-F07, E2-10-F08, E2-10-F09, E2-10-F10; pending IDs: none.
- Logical-cycle snapshots in the latest verification record: 169/169 matched across 587560 state-field comparisons.
- Prototype: INTEGPRB: six zero-speed cycles with L0/S0/T1/C1-C6/T2-save/T3-reload/F0 checkpoints; same-Pascal-file load and round-trip save; not one of the ten fixtures with 41690 total field comparisons across 12 checkpoints; per-checkpoint counts: variable: 3,473 at L0/S0/T2/T3/F0; 3,475 at T1 and each cycle; this is not counted as a complete E2-10 fixture.
- Logical sound-queue events in the prototype: 2 events / 20 event fields; checkpoint sound state: compared; overflow: False.
- Ordered playfield cell writes in the prototype: 9 events / 36 event fields; capture: matched; generated oracle sidecar SHA-256: 6f153a1c437a476912a973144f6ee3e8e483b6478dbc16b9039e58bad0d246fa.
- Generated `.SAV` comparison: 565/565 stable bytes matched; 396 classified positions (327 differing values); raw file size/hash: 961 bytes / `378a6aaae4ef3f8f040f720c006a8896c6a52d4b672b599d0367fb3ace704700`. Pascal save error result: no return value; failure-path outcome remains outstanding.
- Same-file reload/round-trip: Pascal WorldLoad=True, production zoo_world_load=0; 949/949 loaded-file bytes preserved exactly; 12 process pointer bytes classified; input and second-save hashes: `378a6aaae4ef3f8f040f720c006a8896c6a52d4b672b599d0367fb3ace704700` / `8b4d70ab16e3c730f592197a508b409d0bf0e6e244491bd979b738e75466b0e3`.
- Per-cycle checkpoint comparisons: implemented.
- State-transition checkpoints: implemented.
- First-divergence reporting: implemented.
- First-divergence diagnostic negative control: passed; First divergence at cycle 1, field 0 (ammo): Pascal=1, production=0; state summaries and surrounding field/input context were emitted. This instrumentation check receives no replay-fixture or compatibility-case completion credit.
- `E2-10-F01` DEMO start and ordinary traversal: **passed** in the latest run; 12 checkpoints / 41604 state fields; 0 state differences; 0 sound events; 5 cell writes; 25241/25241 stable generated-save bytes and 25241/25241 round-trip bytes matched.
- `E2-10-F02` walls, pushers, duplicators, and collision ordering: **passed** in the latest run; 12 checkpoints / 42414 state fields; 0 state differences; 4 sound events; 16 cell writes; 643/643 stable generated-save bytes and 1051/1051 round-trip bytes matched.
- `E2-10-F03` combat, bullets, keys/doors, and pickups: **passed** in the latest run; 12 checkpoints / 41158 state fields; 0 state differences; 7 sound events; 664 cell writes; 558/558 stable generated-save bytes and 876/876 round-trip bytes matched.
- `E2-10-F04` object stat order plus multiple interacting OOP scripts: **passed** in the latest run; 12 checkpoints / 41940 state fields; 0 state differences; 0 sound events; 5 cell writes; 598/598 stable generated-save bytes and 962/962 round-trip bytes matched.
- `E2-10-F05` board edges, neighbor transitions, and passages: **passed** in the latest run; 15 checkpoints / 51562 state fields; 0 state differences; 1 sound events; 6006 cell writes; 896/896 stable generated-save bytes and 1556/1556 round-trip bytes matched.
- `E2-10-F06` dark room, torch, Energizer, timers, and death: **passed** in the latest run; 126 checkpoints / 438702 state fields; 0 state differences; 4 sound events; 3730 cell writes; 552/552 stable generated-save bytes and 870/870 round-trip bytes matched.
- `E2-10-F07` pause, zero/positive-speed scheduling, and delayed input: **passed** in the latest run; 13 checkpoints / 44226 state fields; 0 state differences; 0 sound events; 4 cell writes; 461/461 stable generated-save bytes and 829/829 round-trip bytes matched.
- `E2-10-F08` save, restore, world reload, and board change: **passed** in the latest run; 17 checkpoints / 58328 state fields; 0 state differences; 1 sound events; 6006 cell writes; 896/896 stable generated-save bytes and 1562/1562 round-trip bytes matched.
- `E2-10-F09` editor mutation, .BRD round-trip, and play of the edited board: **passed** in the latest run; 12 checkpoints / 40872 state fields; 0 state differences; 0 sound events; 5 cell writes; 750/750 stable generated-save bytes and 1152/1152 round-trip bytes matched.
- `E2-10-F10` malformed-file recovery and high-score insertion path: **passed** in the latest run; 7 checkpoints / 24015 state fields; 0 state differences; 0 sound events; 29 cell writes; 610/610 stable generated-save bytes and 858/858 round-trip bytes matched; malformed-load post-failure state 3385/3385 comparable fields matched (0 differences; excluded: World.BoardLen[0] (private raw-buffer length)).
- DEMO start and ordinary traversal: 12 checkpoints; 41604 state-field comparisons total; passed (D+DP+I evidence).
- walls, pushers, duplicators, and collision ordering: 12 checkpoints; 42414 state-field comparisons total; passed (D+DP+I evidence).
- combat, bullets, keys/doors, and pickups: 12 checkpoints; 41158 state-field comparisons total; passed (D+DP+I evidence).
- object stat order plus multiple interacting OOP scripts: 12 checkpoints; 41940 state-field comparisons total; passed (D+DP+I evidence).
- board edges, neighbor transitions, and passages: 15 checkpoints; 51562 state-field comparisons total; passed (D+DP+I evidence).
- dark room, torch, Energizer, timers, and death: 126 checkpoints; 438702 state-field comparisons total; passed (D+DP+I evidence).
- pause, positive-speed scheduling, and delayed input: 13 checkpoints; 44226 state-field comparisons total; passed (D+DP+I evidence).
- save, restore, world reload, and board change: 17 checkpoints; 58328 state-field comparisons total; passed (D+DP+I evidence).
- editor mutation, .BRD round-trip, and play of the edited board: 12 checkpoints; 40872 state-field comparisons total; passed (D+DP+I evidence).
- malformed-file recovery and high-score insertion path: 7 checkpoints; 24015 state-field comparisons total; passed (D+DP+I evidence); malformed-load post-failure state 3385/3385 comparable fields matched (0 differences; excluded: World.BoardLen[0] (private raw-buffer length)).
  - L0: 3467 fields; matched.
  - S0: 3467 fields; matched.
  - T1: 3467 fields; matched.
  - C1: 3467 fields; matched.
  - C2: 3467 fields; matched.
  - C3: 3467 fields; matched.
  - C4: 3467 fields; matched.
  - C5: 3467 fields; matched.
  - C6: 3467 fields; matched.
  - T2: 3467 fields; matched.
  - T3: 3467 fields; matched.
  - F0: 3467 fields; matched.
  - L0: 3501 fields; matched.
  - S0: 3501 fields; matched.
  - T1: 3501 fields; matched.
  - C1: 3501 fields; matched.
  - C2: 3555 fields; matched.
  - C3: 3555 fields; matched.
  - C4: 3555 fields; matched.
  - C5: 3555 fields; matched.
  - C6: 3555 fields; matched.
  - T2: 3545 fields; matched.
  - T3: 3545 fields; matched.
  - F0: 3545 fields; matched.
  - L0: 3419 fields; matched.
  - S0: 3419 fields; matched.
  - T1: 3458 fields; matched.
  - C1: 3458 fields; matched.
  - C2: 3427 fields; matched.
  - C3: 3427 fields; matched.
  - C4: 3427 fields; matched.
  - C5: 3433 fields; matched.
  - C6: 3433 fields; matched.
  - T2: 3419 fields; matched.
  - T3: 3419 fields; matched.
  - F0: 3419 fields; matched.
  - L0: 3495 fields; matched.
  - S0: 3495 fields; matched.
  - T1: 3495 fields; matched.
  - C1: 3495 fields; matched.
  - C2: 3495 fields; matched.
  - C3: 3495 fields; matched.
  - C4: 3495 fields; matched.
  - C5: 3495 fields; matched.
  - C6: 3495 fields; matched.
  - T2: 3495 fields; matched.
  - T3: 3495 fields; matched.
  - F0: 3495 fields; matched.
  - L0: 3402 fields; matched.
  - S0: 3402 fields; matched.
  - T1: 3401 fields; matched.
  - C1: 3402 fields; matched.
  - T2 (neighbor entry): 3418 fields; matched.
  - C2: 3419 fields; matched.
  - T3 (passage): 3467 fields; matched.
  - T4: 3467 fields; matched.
  - C3: 3468 fields; matched.
  - C4: 3468 fields; matched.
  - C5: 3468 fields; matched.
  - C6: 3468 fields; matched.
  - T2 (post-save): 3437 fields; matched.
  - T3 (post-load): 3437 fields; matched.
  - F0: 3438 fields; matched.
  - L0: 3402 fields; matched.
  - S0: 3402 fields; matched.
  - T1: 3425 fields; matched.
  - C1: 3425 fields; matched.
  - C2: 3425 fields; matched.
  - C3: 3531 fields; matched.
  - C4: 3531 fields; matched.
  - C5: 3531 fields; matched.
  - C6: 3531 fields; matched.
  - C7: 3531 fields; matched.
  - C8: 3531 fields; matched.
  - C9: 3531 fields; matched.
  - C10: 3531 fields; matched.
  - C11: 3531 fields; matched.
  - C12: 3531 fields; matched.
  - C13: 3531 fields; matched.
  - C14: 3531 fields; matched.
  - C15: 3531 fields; matched.
  - C16: 3531 fields; matched.
  - C17: 3531 fields; matched.
  - C18: 3531 fields; matched.
  - C19: 3531 fields; matched.
  - C20: 3531 fields; matched.
  - C21: 3531 fields; matched.
  - C22: 3531 fields; matched.
  - C23: 3531 fields; matched.
  - C24: 3531 fields; matched.
  - C25: 3531 fields; matched.
  - C26: 3531 fields; matched.
  - C27: 3531 fields; matched.
  - C28: 3531 fields; matched.
  - C29: 3531 fields; matched.
  - C30: 3531 fields; matched.
  - C31: 3531 fields; matched.
  - C32: 3531 fields; matched.
  - C33: 3531 fields; matched.
  - C34: 3531 fields; matched.
  - C35: 3531 fields; matched.
  - C36: 3531 fields; matched.
  - C37: 3531 fields; matched.
  - C38: 3531 fields; matched.
  - C39: 3531 fields; matched.
  - C40: 3531 fields; matched.
  - C41: 3531 fields; matched.
  - C42: 3531 fields; matched.
  - C43: 3531 fields; matched.
  - C44: 3531 fields; matched.
  - C45: 3531 fields; matched.
  - C46: 3531 fields; matched.
  - C47: 3531 fields; matched.
  - C48: 3531 fields; matched.
  - C49: 3531 fields; matched.
  - C50: 3531 fields; matched.
  - C51: 3531 fields; matched.
  - C52: 3531 fields; matched.
  - C53: 3531 fields; matched.
  - C54: 3531 fields; matched.
  - C55: 3531 fields; matched.
  - C56: 3531 fields; matched.
  - C57: 3531 fields; matched.
  - C58: 3531 fields; matched.
  - C59: 3531 fields; matched.
  - C60: 3531 fields; matched.
  - C61: 3531 fields; matched.
  - C62: 3531 fields; matched.
  - C63: 3531 fields; matched.
  - C64: 3531 fields; matched.
  - C65: 3531 fields; matched.
  - C66: 3531 fields; matched.
  - C67: 3433 fields; matched.
  - C68: 3433 fields; matched.
  - C69: 3433 fields; matched.
  - C70: 3433 fields; matched.
  - C71: 3433 fields; matched.
  - C72: 3433 fields; matched.
  - C73: 3433 fields; matched.
  - C74: 3433 fields; matched.
  - C75: 3433 fields; matched.
  - C76: 3433 fields; matched.
  - C77: 3433 fields; matched.
  - C78: 3433 fields; matched.
  - C79: 3433 fields; matched.
  - C80: 3433 fields; matched.
  - C81: 3433 fields; matched.
  - C82: 3433 fields; matched.
  - C83: 3433 fields; matched.
  - C84: 3433 fields; matched.
  - C85: 3433 fields; matched.
  - C86: 3433 fields; matched.
  - C87: 3433 fields; matched.
  - C88: 3433 fields; matched.
  - C89: 3433 fields; matched.
  - C90: 3433 fields; matched.
  - C91: 3433 fields; matched.
  - C92: 3433 fields; matched.
  - C93: 3433 fields; matched.
  - C94: 3433 fields; matched.
  - C95: 3433 fields; matched.
  - C96: 3433 fields; matched.
  - C97: 3433 fields; matched.
  - C98: 3433 fields; matched.
  - C99: 3433 fields; matched.
  - C100: 3433 fields; matched.
  - C101: 3433 fields; matched.
  - C102: 3433 fields; matched.
  - C103: 3433 fields; matched.
  - C104: 3433 fields; matched.
  - C105: 3433 fields; matched.
  - C106: 3433 fields; matched.
  - C107: 3433 fields; matched.
  - C108: 3433 fields; matched.
  - C109: 3433 fields; matched.
  - C110: 3433 fields; matched.
  - C111: 3433 fields; matched.
  - C112: 3433 fields; matched.
  - C113: 3433 fields; matched.
  - C114: 3433 fields; matched.
  - C115: 3433 fields; matched.
  - C116: 3433 fields; matched.
  - C117: 3433 fields; matched.
  - C118: 3433 fields; matched.
  - C119: 3433 fields; matched.
  - C120: 3433 fields; matched.
  - T2: 3419 fields; matched.
  - T3: 3419 fields; matched.
  - F0: 3419 fields; matched.
  - L0: 3402 fields; matched.
  - S0: 3402 fields; matched.
  - T1: 3402 fields; matched.
  - C1: 3402 fields; matched.
  - T2: 3402 fields; matched.
  - C2: 3402 fields; matched.
  - T3: 3402 fields; matched.
  - C3: 3402 fields; matched.
  - C4: 3402 fields; matched.
  - C5: 3402 fields; matched.
  - T2: 3402 fields; matched.
  - T3: 3402 fields; matched.
  - F0: 3402 fields; matched.
  - L0: 3402 fields; matched.
  - S0: 3402 fields; matched.
  - T1: 3401 fields; matched.
  - C1: 3402 fields; matched.
  - T2: 3418 fields; matched.
  - C2: 3419 fields; matched.
  - T3: 3467 fields; matched.
  - T4: 3467 fields; matched.
  - C3: 3468 fields; matched.
  - C4: 3468 fields; matched.
  - C5: 3468 fields; matched.
  - C6: 3468 fields; matched.
  - T2: 3437 fields; matched.
  - T3: 3401 fields; matched.
  - T4: 3437 fields; matched.
  - T5: 3401 fields; matched.
  - F0: 3402 fields; matched.
  - L0: 3406 fields; matched.
  - S0: 3406 fields; matched.
  - T1: 3406 fields; matched.
  - C1: 3406 fields; matched.
  - C2: 3406 fields; matched.
  - C3: 3406 fields; matched.
  - C4: 3406 fields; matched.
  - C5: 3406 fields; matched.
  - C6: 3406 fields; matched.
  - T2: 3406 fields; matched.
  - T3: 3406 fields; matched.
  - F0: 3406 fields; matched.
  - L0: 3421 fields; matched.
  - S0: 3421 fields; matched.
  - C1: 3421 fields; matched.
  - C2: 3438 fields; matched.
  - T2: 3438 fields; matched.
  - T3: 3438 fields; matched.
  - F0: 3438 fields; matched.

## Mismatches, limitations, and remaining work

- Known unexplained mismatches in the latest recorded run: **0**. This count does not imply complete coverage; unexplained mismatch triage is still a release gate.

### Mandatory remaining work

- E2-01: complete source-to-shared-engine routine mapping; centralize remaining editor mutations and normalize source inventory scope exclusions.
- E2-02: complete the frozen byte/error boundary matrix for all four formats, including per-field serialization cases, applicable truncation boundaries, returned errors, reset/preservation, and untouched-buffer suffix comparisons. The short-payload and controlled one-byte short-header EditorTransferBoard traces now have D/DP evidence; keep E2-02 open until the remaining frozen matrix passes.
- E2-03: finish lifecycle-transition cases and the exact transition pass gate. Player-abort decisions, accepted-abort boundary (22/22), refused No/Escape aftermath (44/44), the title-P refused-abort/zero-speed-cycle/accepted-abort/return-to-title route (3 checkpoints / 10,242 fields), six edge-callback outcomes (60/60 under service stubs) plus six full production board-edge routes (60/60 through actual BoardChange, serialized BoardClose/Open, call-stack continuation, and BoardEnter), title quit, unsupported-version refusal, empty-file refusal, cancel/retry, and missing-world reload now have partial D/DP evidence; remaining save/return outcomes and the broader successful/refused board/world transition matrix remain mandatory.
- E2-04 through E2-08: close remaining element, OOP, allocator/alias, input/RNG, virtual-cell/text-window, and broader sound producer/timer acceptance cases.
- E2-09: complete editor board/property/stat/object/copy-paste mutations and their direct Pascal comparisons; high-score table byte semantics are shared and directly verified.
- E2-11: after E2-01 through E2-10 close, complete the fresh staging run, repeat heap-sensitive cases, and publish the completion record.

### Non-blocking limitations and optional investigations

- Physical Android device testing, UI rendering, platform audio, and frontend adapters are outside Engine 2.0 scope.
- Allocator reuse, unread destination tails, BIOS time, and physical sound output follow the explicit environment-dependent compatibility contract in ENGINE_2.0_CHECKLIST.md.

## Known discrepancies and unresolved questions

- Short-payload .BRD editor import directly reaches BoardCreate and matches eight selected production board/player fields. A controlled SHORTHD.BRD one-byte 03 EditorTransferBoard run now captures the reopened prior board with visible BoardLen 259 and cleared neighbor links; production matches eight selected state fields, board-name bytes, and all four neighbor values. Prior uncontrolled allocator attempts remain recorded but are not counted. Other allocator histories, untouched suffix behavior, and the broader truncation/error boundary matrix remain open; see tools/compatibility/evidence/2026-11-27-e2-02-short-header-controlled-reopen.md.
- Player element metadata differs: Pascal mutates ElementDefs[4].Character while LibZoo uses a draw callback. Resulting virtual-cell behavior across all animation cases remains to be proven.
- Editor flood-fill and .HI table semantics are now shared and directly compared; placement/paste/stat mutation side effects and other Editor.PAS state changes remain outside the platform-neutral API.
- F05 and F08 each counted 834 Pascal and 835 production identical-value redraws; that one-count same-value output cadence remains classified under the existing poll/render environment rule. In F05 state-transition T3, the cumulative effective-cell counter is Pascal 6003 versus production 6004 even though the complete ordered 6006-write stream and every logical-cycle counter match; this derived counter is excluded only at intermediate T checkpoints in F05/F08 and remains recorded for E2-03 transition acceptance. Undefined and process-pointer bytes follow the checklist contract.
- Title malformed-restore comparison: Pascal World.BoardLen[0] is 319 while LibZoo’s augmented private board buffer is 327. This implementation-specific internal size is excluded from equality; the refusal result and remaining 3,385 comparable world/board/stat/tile fields match, and external .ZZT/.SAV bytes are checked by the serializer suite.

## Latest verification record

- Latest full Pascal-runner attempt: exit code 0; Clean Turbo Pascal 5.5/DOSBox-X 2026.10.01 run on source revision a21746dd55bd1ba680a5749491be6838c241fc88: 148 direct Pascal sessions (54 standard, 94 standalone), 66/66 direct Pascal/production comparison invocations (65 existing plus one signed .BRD boundary comparator covering five values), and F01-F10 (238 checkpoints, 824,821 checkpoint fields, 169 logical-cycle snapshots). Every signed boundary decoded in the board time limit and six stat fields across four stat records; all five 289-byte board payloads matched exactly. F10 post-failed-WorldLoad state matched 3,385/3,385 comparable fields with private World.BoardLen[0] excluded. No unexplained comparable mismatch; E2-03-AC001 remains partial and E2-10 remains the only closed blocker.
- Latest integrated Pascal differential `E2-10-F10`: exit 0; 24015 snapshot fields, 0 logical sound events, 29 ordered playfield cell writes, 610/610 generated-save stable bytes, and 858/858 loaded-file round-trip stable bytes matched.
- Toolchain: Borland Turbo Pascal 5.5 (TPC.EXE and TURBO.TPL; excluded from repository); DOSBox-X 2026.10.01; executable SHA-256 B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3; external tool, not redistributed.
- Platform build toolchains: Windows: Visual Studio 2022 MSBuild 17.14.19, Windows SDK 10.0.26100.0, x64; CTest 3/3 passed; Android: Gradle 8.11.1 offline; Android NDK 27.2.12479018; arm64-v8a and x86_64.
- Source revision: LibZoo baseline `7fb3e89a8467e66a4b8cbc290c708b9fa5290886`; local LibZoo overlay `e5d2fe7`; Pascal source hashes in the manifest; root Git revision: a21746dd55bd1ba680a5749491be6838c241fc88.
- Last completed full Pascal suite exit code: 0; command recorded: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain 'build/pascal-compat-run/Z' -DosboxExecutable '<external DOSBox-X 2026.10.01 executable>'; exit 0`.
- Run record: [`latest_verification.json`](tools/compatibility/latest_verification.json).

## Recent progress

- E2-02 AC002 signed .BRD boundaries passed on a21746dd55bd1ba680a5749491be6838c241fc88: the existing compiled-Pascal BOARDENC session emitted five BoardClose files for -32768, -1, 0, 1, and 32767. Production decoded TimeLimitSec plus StepX/StepY/Cycle/Follower/Leader/DataPos for all four stat records and re-emitted each full 289-byte payload byte-for-byte. Clean suite: 148 direct Pascal sessions, 66/66 direct comparisons, F01-F10 at 238 checkpoints / 824,821 fields / 169 cycles; E2-02 remains partial and no acceptance cases were added. Evidence: tools/compatibility/evidence/2026-11-27-e2-02-pascal-signed-brd-boundaries.md.
- E2-02 signed serialization portability fix on e2bbf2d45f535c03b532b309ae6f2eaefd30d6d8: replaced implementation-defined 16/8-bit signed conversions in shared world and .BRD codecs with range-safe Pascal bit interpretation; writers preserve the original wire bytes. Exhaustive production regression passed all 65,536 word patterns and 256 packed-byte patterns. The clean Pascal run passed 148 sessions, 65/65 differentials, and all ten integrated fixtures; runtime, Windows 3/3 CTest, Android compatibility/build, and serializer checks passed. E2-02 AC002 and the blocker remain partial; the frozen inventory did not change. Evidence: tools/compatibility/evidence/2026-11-27-e2-02-portable-signed-serialization.md.
- E2-02-AC003/AC004: controlled one-byte SHORTHD.BRD short-header EditorTransferBoard completed in the original Pascal path using a documented 60,000-byte heap-sink allocation history. Direct output was board 6 41 0 21 47 30 12 259 0 0 0 0; production matched eight selected fields, name bytes, and four neighbors. Clean run: 148 direct Pascal sessions (54 standard/94 standalone), 65/65 differentials, F01-F10 at 238 checkpoints / 824,821 fields / 169 cycles. E2-02 remains partial and the 54-case inventory is unchanged. See tools/compatibility/evidence/2026-11-27-e2-02-short-header-controlled-reopen.md.
- Earlier pre-hook E2-02 checkpoint: BRDXFER used ZZT memory limits and its short-payload differential passed; the corrected one-byte header attempt then timed out without output. This was superseded by the controlled heap-sink D/DP capture in tools/compatibility/evidence/2026-11-27-e2-02-short-header-controlled-reopen.md.
- E2-10-F10: extended the existing malformed-world replay with a pre-fallback state snapshot. Turbo Pascal 5.5 and production matched 3,385/3,385 comparable fields (BoardLen[0] excluded); the clean suite passed 147 sessions, 65/65 differentials, and all ten frozen integrated fixtures. Inventory and corpus totals did not change.
- E2-03-AC001: compiled the original Pascal GameTitleLoop W world-selection route, then P play, one actual paused cycle, and return-to-title; direct Pascal/production replay matched at 3 checkpoints / 10,360 fields. Corrected clean-run totals: 147 direct sessions and 65/65 differentials. E2-10 F01-F10 remain 10/10; the frozen inventory is unchanged and E2-03 remains partial.
- E2-03-AC001: connected BOARDENT direct Pascal captures to two production comparisons. The callback-boundary replay under service stubs matched 60/60 fields; a second replay through actual BoardChange, serialized BoardClose/Open, call-stack continuation, and BoardEnter also matched 60/60. The clean runner passed 146 direct sessions, 64/64 differentials, F01-F10, runtime compatibility, and the first-divergence negative control. External world-file routes and the broader lifecycle matrix remain open; E2-03 stays partial and the frozen inventory is unchanged.
- E2-03-AC001: extended the original GameTitleLoop probe through refused abort, verified zero-speed tick advancement, accepted abort, and return-to-title. The direct Pascal/production replay matched 3 checkpoints / 10,242 fields. The clean run passed 146 direct sessions, 63/63 configured differentials, F01-F10, runtime verification, and the diagnostic negative control; the frozen 54-case inventory remains unchanged and E2-03 stays partial.
- E2-03-AC001: added direct compiled-Pascal refused-abort aftermath captures for N and Escape, each followed through the actual GamePlayLoop zero-speed cycle boundary to accepted exit. Both routes matched production at 44/44 fields total. The clean full runner passed 145 direct sessions, 62/62 configured differentials, and F01-F10; E2-03 remains partial and the frozen 54-case inventory is unchanged.
- E2-03-AC001: added a compiled-Pascal accepted player-abort GamePlayLoop capture and compared the shared-engine exit boundary across 22 fields. The clean full runner exited 0; its direct-session tally is corrected from 140 to 142 to include ABORTLP compile/run. E2-03 remains partial; no case or fixture IDs were added.
- Generalized integrated replay snapshots to live stat counts and unique script owners; the clean Pascal suite passed 84 direct sessions and 43/43 configured differentials. The two-script, three-stat prototype matched 31,158 fields across nine checkpoints. E2-10 remains open at 0/10 fixtures.
- Earlier E2-02 checkpoint (84 direct sessions): short-payload editor transfer matched production; corrected short-header behavior was still unresolved at that time. A later controlled allocator-history direct capture and D/DP comparison is recorded in tools/compatibility/evidence/2026-11-27-e2-02-short-header-controlled-reopen.md.
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
- E2-03-AC002 passed in the clean 2026-11-26 run at 80fd8d8915a5ffff3795d478a7d3f598bf8ffb27: nine deterministic schedule rows / 117 fields matched direct compiled Pascal and shared production for TickSpeed 0, 1, 6, and 9 at phases 0 and 5980, plus the four-PIT stall. Existing F07 supplies pause/resume coverage. E2-03 remains open.
- E2-03-AC003 passed in the clean 2026-11-26 run on source commit 41409f7f2b0cc8a4278ebe426bcdccbbc3f8ca47: all ten frozen integrated fixtures passed (238 checkpoints / 824,821 state fields / 169 logical cycles), with 17 sound events and 16,470 ordered effective cell writes matched. F05/F08 T-checkpoint counter boundaries are explicitly classified; per-cycle C counters and global write streams remain exact.
- E2-03-AC001 title lifecycle follow-up passed in the clean 2026-11-26 run at source revision dbf210e2b33b13ff472dbbc1e808e063ed7ebd18: 130/130 direct Pascal sessions and 56/56 differentials. Original GameTitleLoop P and R routes each matched three checkpoints (10,242 P fields; 10,287 R fields), including one real paused gameplay cycle and return-to-title. R consumed the exact 1,649-byte Pascal-generated .SAV. F01-F10 remain 10/10 at 238 checkpoints / 824,821 fields / 169 logical cycles. AC001 remains partial: refusal/cancel/retry, quit/abort, and the remaining successful/refused transition matrix are open.
- E2-03-AC001 title restore refusal passed in the clean 2026-11-26 run at source revision 9b7c24568b6d083ee38376ab78058bb2dd0f6c66: 131/131 direct Pascal sessions and 57/57 configured differentials. Original GameTitleLoop R refused an unsupported-version save; 3,385/3,386 comparable fields matched, and the shared restore path now preserves the source-defined title player cell. F01-F10 remain 10/10 at 238 checkpoints / 824,821 fields / 169 cycles. AC001 remains partial: cancellation, retry, quit/abort, remaining save/return routes, and the consistent transition matrix are open.
- E2-03-AC001 clean rerun on source commit b79bf3dd28ced020d66635191d987dc784b8b08b: direct Pascal title restore cancellation followed by retrying R matched production at four checkpoints / 13,716 state fields using the same Pascal-generated .SAV. Full suite passed 134 direct sessions, 58/58 configured differentials, F01-F10 (238 checkpoints / 824,821 fields / 169 logical cycles), and the runtime suite; AC001 remains partial because quit/abort and the full transition matrix are still open.
- E2-03-AC001 player-abort/title-quit decision follow-up passed in the clean 2026-11-26 run on source revision 8722ef11788369a7e53fe86dd28340205712bd2a: direct Pascal GamePromptEndPlay Yes/No/Escape outcomes at both pause states plus the health-zero path matched shared engine results 7/7; SidebarPromptYesNo title quit Yes/No/Escape matched 3/3. Full runner passed 136 direct sessions, 59/59 configured differentials, F01-F10 (238 checkpoints / 824,821 fields / 169 logical cycles), runtime suite, and first-divergence negative control. AC001 remains partial; E2-10 remains the only closed blocker.
- E2-03-AC001 missing-world reload preservation passed in the clean 2026-11-26 run at source commit de38335ae97cf1eee83c3ebedf9d4a84d4e0b3ac: direct compiled Pascal WorldLoad returned false, shared zoo_world_reload returned ZOO_ERROR_INVAL (-2), and 18 preserved world/board fields matched. Full runner passed 138 direct Pascal sessions and 60 direct differential invocations (59 existing comparisons plus this explicit comparator), F01-F10, runtime checks, and first-divergence negative control. AC001 remains partial because remaining save/return outcomes and the consistent successful/refused transition matrix remain mandatory.
- E2-03 missing backing-world title-P path was rerun in the clean 2026-11-26 suite on source commit 58c01427d7095ec94ecefaec9948cbe42dda1774. The direct Pascal WorldLoad capture returned false; production zoo_world_play returned ZOO_ERROR_INVAL (-2) before play startup; all 18 preserved world/board fields matched. Full run passed 138 direct sessions, 60 direct differentials, F01-F10 (238 checkpoints / 824,821 fields / 169 cycles), runtime checks, and first-divergence negative control. AC001 remains partial; other save/return routes and the consistent success/refusal transition matrix remain mandatory.
- E2-03 successful title-P backing-world reload passed in the clean 2026-11-26 runner: direct Pascal GameTitleLoop P loaded DEMO.ZZT after a title-only setup, and production zoo_world_play reloaded the same bytes; three checkpoints / 10,360 state fields matched through one real paused cycle and return-to-title. Full run passed 139 direct Pascal sessions (54 standard, 85 standalone), 61/61 direct differentials, F01-F10 (238 checkpoints / 824,821 fields / 169 cycles), runtime checks, and the first-divergence negative control. AC001 remains partial because other save/return outcomes and the broader successful/refused transition matrix remain mandatory.
- E2-03-AC001 empty-file refusal passed in the clean 2026-11-26 runner on source revision 4a10f5b33963108128b6b11dae5d1646e0d09979: direct original GameTitleLoop R refusal against an existing zero-byte RESTORE.SAV matched 3,384/3,386 comparable fields; field 2 private board-buffer length and field 16 frontend-consumed InputKeyPressed are excluded. Production returned ZOO_ERROR_INVAL (-2). Full run passed 140 direct sessions, 62/62 differentials, F01-F10, runtime suite, and first-divergence negative control; AC001 remains partial.
- After the clean 140-session Pascal/production run, Android CP437/key verification and debug plus Android-test APK builds passed for arm64-v8a and x86_64; Windows Release x64 build and CTest passed 3/3 after shared-engine commit 4a10f5b33963108128b6b11dae5d1646e0d09979. No Android device test was performed.

Generated by `python tools/compatibility/generate_progress.py`. Do not edit this report manually.
