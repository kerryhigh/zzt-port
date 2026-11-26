# E2-10-F07 pause, positive-speed scheduler, and delayed input

**Result: passed.** This is the seventh completed fixture in the frozen ten-fixture E2-10 corpus. F08–F10 remain pending, so E2-10 is still a release blocker.

## Direct oracle and production replay

`PAUSEIN.PAS` constructs a deterministic board and invokes the reconstructed Pascal `GamePlayLoop` with the original `InputUpdate` and player callback. The test-only normalized input tape is `RPNNRNNN`; the positive-speed scheduler uses `TickSpeed=6`, `TickTimeDuration=12`, and a deterministic logical-clock increment of 5.4924578 hundredths per scheduler poll. The tape exercises ordinary movement, a delayed pause key, movement while paused, and resume. The final player is healthy at `(7,5)` and the game is unpaused. The production side runs the shared `zoo_tick` and `zoo_input_update` implementation with the same setup and tape.

The replay records initial state, loop start, each relevant state transition, all five cycles, save/load state, and final state. The checkpoint tags are `L0, S0, T1, C1, T2, C2, T3, C3, C4, C5, T2, T3, F0` (the post-loop save/load capture reuses the `T2`/`T3` tags). All 13 checkpoints contain 3,399 fields and match: **44,187 state-field comparisons, zero differences**.

| Comparison | Result |
|---|---:|
| Logical sound events | 0; no differences |
| Ordered effective cell changes | 4 events / 16 fields; no differences |
| Identical-value redraws | Pascal 3, production 3 |
| Fresh `.SAV` | 833 bytes; 461 stable bytes matched; 372 undefined/pointer positions classified; zero stable or classified differences |
| Loaded-file round-trip `.SAV` | 829 preserved bytes matched; four process-pointer positions classified; zero stable-byte differences |
| Load outcome | Pascal `WorldLoad=true`; production `zoo_world_load=0` |

The cell capture keeps redundant output writes separate from effective cell changes. For F07 those counts match exactly. The distinct F05 count of 834 Pascal versus 835 production redraws remains separately recorded under the existing clock/poll cadence rule.

## Evidence classes and interventions

- **D:** `PAUSEIN.PAS` was compiled and run against the reconstructed Pascal gameplay source using Borland Turbo Pascal 5.5 under DOSBox-X.
- **DP:** the captured state, sound queue, ordered cell changes, and `.SAV` stable-byte positions were compared against the production shared-engine replay by `compare-integrated-gameplay.js`.
- **I:** this uses the real Pascal `GamePlayLoop` and shared production `zoo_tick` over the full five-cycle scenario; it does not substitute a separate gameplay model.

The oracle hooks only control or observe environmental inputs: `RandSeed=0`; transition-table generation before `WorldCreate`; `IoTmpBuf` allocated before world initialization; `World.Info.unk1=0` because that word is undefined after `WorldCreate`; a staged seed for otherwise undefined `pauseBlink`; the normalized key tape; a deterministic scheduler clock; and captured `VideoWriteText`/logical `SoundQueue` events. Physical audio output is disabled. The original game loop and its gameplay callbacks remain in control. Compilation and execution occur in separate fresh DOSBox-X sessions to control inherited allocation history. Oracle instrumentation is staged by `tools/pascal-compat/verify.ps1`; reconstructed reference gameplay semantics are not changed.

## Reproduction record

The full clean Pascal/production runner exited 0 on 2026-11-25 at project revision `2adbf0bb4021533f0477217003164749921d819f`. It passed **121/121 direct Pascal sessions** and **51/51 configured Pascal-to-production comparisons**, including F01–F07. The integrated diagnostic negative control and runtime compatibility verifier also passed. The focused F07 comparator reported 13/13 checkpoints and 44,187/44,187 state fields matched. F07 contributes one fixture completion credit; E2-10 is **7/10**.

The reconstructed source hashes are recorded in [`reference_source_hashes.sha256`](../reference_source_hashes.sha256). The shared LibZoo implementation is based on upstream version 1.0 commit `7fb3e89a8467e66a4b8cbc290c708b9fa5290886`, with the tested local overlay at `e5d2fe7`. Toolchain: Turbo Pascal 5.5 (external, proprietary, not distributed); DOSBox-X 2026.10.01, executable SHA-256 `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`; Node.js 22.19.0; Python 3.13.7. The DOSBox-X executable and compiler are external dependencies and are not included in this repository.

Oracle files were emitted into the ignored clean staging directory and are not committed as build artifacts:

| File | Bytes | SHA-256 |
|---|---:|---|
| `INTEGPR.OUT` | 93,020 | `b428274a4cdf8011b2d2c7e65e621ce5d4069b42bff2a10390dbfec6b15603e7` |
| `INTEGSND.OUT` | 5 | `043b5ae7334040ef63298adca08912d5287ad37e66f9425316adc50d2c1ab652` |
| `INTEGCEL.OUT` | 49 | `7ec1f5425ab34de63e7132b83f8554b34df5542c0b2aadac8b95675e79f1e9b8` |
| `INTEGSAV.SAV` | 833 | `faac8906f433045b02f9e00223d587941c258d10f7d6bda8936fc493622af1a7` |
| `INTEGLOD.OUT` | 3 | `f1b2f662800122bed0ff255693df89c4487fbdcf453d3524a42d4ec20c3d9c04` |
| `INTEG2SV.SAV` | 833 | `faac8906f433045b02f9e00223d587941c258d10f7d6bda8936fc493622af1a7` |

