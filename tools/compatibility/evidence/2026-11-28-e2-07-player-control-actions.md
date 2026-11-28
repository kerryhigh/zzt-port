# E2-07-AC002: player control and menu actions

**Date:** 2026-11-28  
**Acceptance case:** E2-07-AC002 only; the frozen inventory remains at 54 acceptance cases and E2-07 remains open.  
**Evidence:** direct compiled Pascal (**D**) plus direct differential (**DP**).  
**Tested code:** local commit `d88c5e9a3ccf72e8222ba932175f504ac334a159`, tree `446eeafc00c3985a6c474da89af4020edb62ef8e`; published code mirror `52484d447aabc9ace8965e67d24a2a159f84ef99`. Reference source files are unchanged and their hashes are recorded in [`reference_source_hashes.sha256`](../reference_source_hashes.sha256).

## Coverage

The existing `compare-player-tick.js` compares Pascal output from the original `ElementPlayerTick` cases with the shared production player callback:

- Direction and movement: four empty, blocked, push, and blocked-push cases.
- Shoot held/tap: five cases cover a held fire action while moving, a one-shot key with a remembered direction, blocked firing, maximum shots, and a one-shot key without a direction. The comparison includes ammo, bullet/stat creation, direction, sound queue, and target cells.
- Pause and sound keys: the original `p` pause and `b` sound-toggle cases.
- Death reset: two health-zero cases, including held movement/fire plus a queued Space key and an existing stat at `(0,0)`.
- Paused movement: three `GamePlayLoop` cases compare an open move, the following neutral player tick, and a blocked move. The only ignored field is `GamePlayExitRequested`, which the probe sets to stop the original loop after capture.

These rows compare 243 fields. `compare-player-abort.js` adds the original Yes/No/Escape end-play prompt in both pause states plus the no-prompt death path (35 fields), title quit Yes/No/Escape decisions (6 fields), the accepted abort through actual `GamePlayLoop` return (22 fields), and No/Escape refusal followed by accepted exit through the loop (44 fields). Those rows compare 107 fields. The two focused comparators both exited 0; all 350 directly compared fields matched.

The real-loop F07 replay separately exercises pause/resume with delayed input over five logical cycles. It matched 13/13 checkpoints and 44,226 state fields, including all five cycle checkpoints, four ordered cell writes, 461 stable save bytes, and 829 round-trip bytes. F07 remains one of the existing ten E2-10 fixtures; this case does not add integrated-fixture credit or duplicate its count.

## Oracle interventions and evidence limits

The player probes set controlled world/player state and replace only `VideoWriteText` with a quiet capture stub. The end-play and title prompts use a test-only queued `ReadKey` response; the accepted/refused loop probes use deterministic queued responses and a controlled timer tape. The original `ElementPlayerTick`, prompt routines, `GamePlayLoop`, and production `zoo_tick` remain in control. These environment hooks and setup are existing verifier instrumentation; no reference gameplay semantics were changed.

The separate lower-level `zoo_input_shoot_requested` assertions in `tools/runtime-compat/player_input.c` are production-only and receive no D/DP credit here. The direct Pascal cases compare the mapped held/tap action through the actual player callback; broader accounting of every shared input mutation path remains open under E2-07-AC005.

## Oracle outputs

| File | Bytes | SHA-256 |
|---|---:|---|
| `PLAYERMV.OUT` | 396 | `7991ea360a63940b7334aff1b2ea84262a3e83d1045995e9385a4df3162040d0` |
| `PLAYERST.OUT` | 465 | `6663dc8753de7b5321ab259c6524b9a2300f69d3faae39ecaf5fb7c50cd675a7` |
| `PLAYERKY.OUT` | 179 | `ece0a4a71fa115562d863ccedce49f4acaa0bc0a917a9b338db9443c83728914` |
| `PLAYERDE.OUT` | 272 | `39833714d983774a7d749a3cf3238e605966f4a9910ef9aac513ebbc950d53e8` |
| `PAUSEMV.OUT` | 316 | `15bed9fdda843b520ddad393d23f906322fb18748325b10f32c140ee873081a9` |
| `ABORT.OUT` | 83 | `b81bbc6d066e636bf3eec8a236376cf2f7bf51c8af94314716899b37a00e9511` |
| `TQUIT.OUT` | 15 | `183886d371228921f1f961df371ee891d14ec00cc771f6f376d389964697aea4` |
| `ABORTLP.OUT` | 51 | `faef2d8e8ee74b138178b061437af70dacc8f792eca498d155fb0b731c680f65` |
| `ABORTN.OUT` / `ABORTE.OUT` | 51 each | `d51169e041296f0d5b093250239f464f5a101c88a9668d5e64a79928b63d06f9` each |

The full clean Turbo Pascal 5.5/DOSBox-X 2026.10.01 runner on the tested code revision passed 150/150 direct sessions, 75/75 configured Pascal-to-production comparisons, and F01–F10 with no unexplained comparable mismatches. The focused commands were:

```powershell
node tools/pascal-compat/compare-player-tick.js build/pascal-compat-verification build/runtime-compatibility/oop_movement.wasm
node tools/pascal-compat/compare-player-abort.js build/pascal-compat-verification/ABORT.OUT build/runtime-compatibility/integrated_gameplay.wasm build/pascal-compat-verification/TQUIT.OUT build/pascal-compat-verification/ABORTLP.OUT build/pascal-compat-verification/DEMO.ZZT build/pascal-compat-verification/ABORTN.OUT build/pascal-compat-verification/ABORTE.OUT
```
