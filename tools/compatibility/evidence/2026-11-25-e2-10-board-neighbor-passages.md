# E2-10-F05: board neighbors and passages

## Result

**Passed as the fifth required E2-10 integrated fixture.** The frozen corpus is 5/10 complete; F06–F10 remain open, and E2-10 remains a release blocker.

`BOARDPASSIN.PAS` constructs three boards and exercises the original `BoardChange`, `BoardEnter`, passage callback, and six-cycle `GamePlayLoop`. The player exits west from board 0 into board 1 and appears at its west entry `(60,5)`. A matching passage moves the player to board 2 at `(22,17)`, confirming the original last-matching-passage scan while the earlier matching passage at `(20,15)` remains. After resuming, the player moves to `(23,17)`.

| Evidence | Result |
|---|---:|
| Named checkpoints | 15: `L0`, `S0`, `T1`, `C1`, `T2`, `C2`, `T3`, `T4`, `C3`–`C6`, post-save `T2`, post-load `T3`, `F0` |
| State fields compared | 51,493; 0 differences |
| Logical sound queue | 1 event / 66 fields; 0 differences |
| Ordered effective playfield cell changes | 6,006 / 24,024 fields; 0 differences |
| Same-value redraws | Pascal 834; production 835; counted and reported separately under the documented poll/render cadence contract |
| Generated `.SAV` | 1,580 bytes; 896 stable bytes matched; 684 undefined/pointer positions classified; 0 stable or classified differences |
| Same-file load and second save | Pascal `WorldLoad=true`; production `zoo_world_load=0`; 1,556 preserved bytes matched; 24 process-pointer positions classified |
| Unexplained fixture mismatches | 0 |

The one-count difference is an additional redundant pause-cursor write that leaves its destination cell's character and attribute unchanged. The replay compares the ordered effective cell-state changes, complete engine checkpoints, sound events, and stable file bytes. It reports identical-value write counts separately because their count follows polling/render cadence; a changed cell value, effective-write order, engine state, or tick outcome is not normalized.

## Reproduction and verification

The clean full run used the external Turbo Pascal 5.5 and DOSBox-X toolchain:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain build/pascal-compat-run/Z -DosboxExecutable "<DOSBox-X 2026.10.01 executable>"
```

The clean runner exited 0 with 119 direct Pascal probe sessions and 49/49 configured Pascal-to-production comparisons. The runtime compatibility verifier passed. Code and capture harness tested: local commit `af5929bf67fc32c1600d90582a98792db4452bcf`. Turbo Pascal and DOSBox-X executables are external, proprietary or separately licensed dependencies and are not included.

## Controlled setup and capture

- Set `RandSeed=0`, call `GenerateTransitionTable` before `WorldCreate` as ZZT startup does, and set `World.Info.unk1=0` because `WorldCreate` leaves this word undefined while production setup uses deterministic storage.
- Construct neighbor, player, and passage state through original Pascal board routines. The two destination passages share color `$2A`, so the source's last-match scan is observable.
- Set zero tick speed and a fixed PIT-equivalent clock using `SoundProbeSetTime(0,0,0,30)`. Queue the normalized `LLLR` input tape and seed the otherwise undefined local `pauseBlink` through the existing test hook.
- Compile `BOARDPASSIN` and execute it in separate fresh DOSBox-X sessions to keep compiler-inherited DOS allocation history out of the run process.
- Capture `VideoWriteText` and `SoundQueue` outputs. The hooks do not replace the original loop, callbacks, or shared production gameplay. The cell observer records effective per-cell changes and also counts repeated writes of the same character and attribute without treating them as visible changes.
- Compare all checkpoint fields, the event streams, fresh save, same-file load result, and round-trip save. Only the documented undefined/pointer positions are classified; all defined save bytes are compared exactly.

## Oracle capture hashes

Generated files are in the ignored `build/pascal-compat-verification` staging directory and are not committed as outputs.

| Output | Size | SHA-256 |
|---|---:|---|
| `INTEGPR.OUT` | 108,638 | `ae820773cc411d5ea965977b083d7e1c679e2c552a5860618d04414fc80b61b4` |
| `INTEGSND.OUT` | 171 | `54ce182b9a2f5d64ea10669833d391589fd937e8ecbf61ac88453a310b0a5c20` |
| `INTEGCEL.OUT` | 75,014 | `f4be88707882d7cd34a963b0efe2f4fc885822f05523ef97c14c6a9b5d69b070` |
| `INTEGSAV.SAV` | 1,580 | `c034768a1dfb2d8013f79c5ae89da27ce74ba01021ca94396511a7da828e86de` |
| `INTEGLOD.OUT` | 3 | `f1b2f662800122bed0ff255693df89c4487fbdcf453d3524a42d4ec20c3d9c04` |
| `INTEG2SV.SAV` | 1,580 | `c034768a1dfb2d8013f79c5ae89da27ce74ba01021ca94396511a7da828e86de` |

## Source and fixture locations

- Reconstructed gameplay and passage source: [`GAME.PAS`](../../../third_party/reconstruction-of-zzt/SRC/GAME.PAS), [`ELEMENTS.PAS`](../../../third_party/reconstruction-of-zzt/SRC/ELEMENTS.PAS).
- Direct compiled-Pascal probe: [`BOARDPASSIN.PAS`](../../pascal-compat/BOARDPASSIN.PAS).
- Production integrated setup and shared gameplay calls: [`oop_movement.c`](../../runtime-compat/oop_movement.c), `run_integrated_board_neighbor_passage_checks`.
- Differential and first-divergence reporting: [`compare-integrated-gameplay.js`](../../pascal-compat/compare-integrated-gameplay.js), scenario `board-neighbors-passages`.
- Clean runner: [`verify.ps1`](../../pascal-compat/verify.ps1).
- Fixture record: [`integrated_replay_fixture_manifest.json`](../integrated_replay_fixture_manifest.json), ID `E2-10-F05`.
- Untouched Pascal reference-source hashes: [`reference_source_hashes.sha256`](../reference_source_hashes.sha256).
