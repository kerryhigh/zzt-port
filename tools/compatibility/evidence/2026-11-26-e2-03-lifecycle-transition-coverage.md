# E2-03-AC001 — lifecycle transition evidence map

**Result: partial.** This maps existing direct compiled-Pascal (**D**) and Pascal-to-production (**DP**) evidence to the frozen AC001 transition list. It does not add acceptance IDs or change the inventory denominator. E2-03 remains partial.

## Existing evidence and gaps

| Transition or condition | Existing evidence | AC001 status |
|---|---|---|
| Initial/full world load | F01 loads `DEMO.ZZT` through Pascal `WorldLoad` and production `zoo_world_load`; F10 directly exercises the malformed-load failure and explicit fallback `WorldCreate` used by startup. | Partial: startup title-only load is only a direct probe; malformed retry is absent. |
| Title-to-play | `TITLEIN.PAS` enters the original Pascal `GameTitleLoop`, supplies the deterministic queued keys `P`, `X`, and records the actual player-start `P/0` checkpoint. The production replay calls `zoo_world_play` and compares the same full state. | Partial: the `P` branch is now D/DP covered. The replay exposed that production discarded `key_pressed` and the remaining input buffer across `zoo_game_start`; `zoo_world_play` now preserves the Pascal-observable key, queue head, and queue count. The `R`/restore branch and successful/refused load outcomes remain open. |
| Board entry/change | `BOARDENT.PAS`, `BOARDIN.PAS`, F01, F05, and F08 exercise selected board entry/change paths. | Partial: cases are not yet one complete transition matrix with checkpoints for every success/refusal branch. |
| Passage travel | F05 runs west-neighbor entry and the original passage callback through the real Pascal game loop and compares production. | Covered for the F05 route; other lifecycle interactions remain open. |
| Death | F06 runs the real loop through timer processing and death, with state, sound, and cell output compared. | Covered for the exercised death route; return/quit aftermath remains open. |
| Restore | F08 saves, mutates live state, reloads `.SAV`, reloads baseline `.ZZT`, and compares snapshots and stable serialized bytes. | Covered for restore/reload operations; not for selecting Restore in the original title menu. |
| Pause/resume | F07 runs the original loop with delayed pause input, movement while paused, and resume; the production loop is compared. | Covered for the exercised pause/resume route. |
| Save/return and return-to-title | F08 verifies save and restore bytes/state. `TITLEIN.PAS` captures `C1` after one real player cycle and `T/0` after the original title loop regains control; production uses `zoo_world_return_title`. | Partial: direct D/DP covers one player-cycle/return-to-title route. Saving before return and the actual title-loop save/return aftermath remain open. |
| Quit/abort | Existing probes cover selected monitor/player key and pause behavior. | Open: no D/DP replay of `GamePromptEndPlay` yes/no/escape behavior or title-loop quit confirmation/cancel. |
| Malformed-load failure | F10 directly runs Pascal `WorldLoad` on the malformed startup fixture and compares the production error class and fallback state. | Covered for the startup failure/fallback route. |
| Retry | No current direct differential re-enters the load selection path after a failed load. | Open. |
| Successful and refused/blocked transitions | `BOARDENT.PAS` and `BOARDIN.PAS` cover selected entry and rollback cases; F05/F08 cover successful changes. | Partial: successful and refused cases are not yet captured under a consistent D/DP transition replay. |

## Reused verification

The latest previously clean Pascal/production run on 2026-11-26 exited 0 with 126/126 direct Turbo Pascal sessions, 54/54 configured differentials, and all ten frozen integrated fixtures passing. F01, F05–F08, and F10 provide the lifecycle evidence listed above. After adding `TITLEIN`, the focused compiled-Pascal differential passed P/player-start, C1/one actual gameplay cycle, and T/title-return: 3 checkpoints and 10,242 state fields matched. Its generated `build/pascal-compat-verification/INTEGPR.OUT` was 50,437 bytes with SHA-256 `fc4d432a6ccbf007b95d50114cb7e3a8cccfd18c9eec2060d088210e0a1d62f1`. A clean full run after the fix is still pending; earlier suite totals do not include TITLEIN and do not mean every AC001 branch passed.

The staged oracle hook suppresses only the blocking About modal for this replay and stops the original loops immediately after the recorded lifecycle checkpoints. `TITLEIN.PAS` initializes deterministic RNG, tick, sound, and memory capture state and sends the actual queued `P`/`X` input through the reconstructed input path. `InputUpdate`'s original buffer truncation leaves/repeats the first queued key in this case; that behavior is retained in both Pascal and production. Instrumentation captures state and bounds the run; it does not replace title or gameplay semantics.

The current production-only `run_game_start_enter_checks` in `tools/runtime-compat/tick_exit.c` is supplemental evidence for title/play initialization and preserved clock fields. It is not counted as D or DP because it has no compiled-Pascal counterpart.

## Remaining work for AC001

- Add direct D/DP checkpoints through the original `GameTitleLoop` `R` branch, including successful and refused loads.
- Compare the saved play-to-title aftermath and title-quit paths, including accepted and canceled confirmation prompts and player abort behavior.
- Exercise failure followed by retry using deterministic input and file availability tapes.
- Compare transition snapshots and ordered engine side effects; reuse the existing integrated snapshot, sound, cell-write, and serialization comparators rather than introducing a separate game model.

