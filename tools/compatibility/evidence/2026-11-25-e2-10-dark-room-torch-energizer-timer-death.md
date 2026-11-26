# E2-10-F06: dark room, torch, Energizer, timers, and death

## Result

**Passed as the sixth required E2-10 integrated replay fixture.** The frozen replay corpus is now 6/10 complete. E2-10 remains open, and no Engine 2.0 release blocker is closed by this fixture alone.

The direct Turbo Pascal 5.5 fixture ran the original `GamePlayLoop` with the reconstructed Pascal element callbacks. Its captured checkpoints, sound queue, effective playfield changes, world-load result, and stable save bytes matched the shared LibZoo production gameplay replay. There were zero unexplained state, event, or stable serialized-byte differences.

| Comparison | Result |
|---|---:|
| Named initial, transition, per-cycle, and final checkpoints | 126; 438,324 state fields compared, 0 differences |
| Logical sound queue | 4 events / 328 event fields, 0 differences |
| Ordered effective playfield cell changes | 3,730 / 14,920 fields, 0 differences |
| Repeated identical-value redraws | Pascal 169; production 169 |
| Generated `.SAV` | 878 bytes; 552 stable bytes matched; 326 undefined or process-pointer positions classified; 0 unexplained differences |
| Same-file load and round-trip `.SAV` | Pascal `WorldLoad=true`; production `zoo_world_load=0`; 870 stable bytes matched; 8 process-pointer positions classified |
| Unexplained fixture mismatches | 0 |

The 120 per-cycle checkpoints compare 3,422 fields at cycles 1–2, 3,528 at cycles 3–66, and 3,430 at cycles 67–120. The remaining six checkpoints are `L0`, `S0`, `T1`, post-save `T2`, post-load `T3`, and `F0`; all matched. The stable file-byte comparisons exclude only the explicitly classified undefined or process-pointer positions under the existing nondeterminism contract.

## Integrated gameplay exercised

`DARKIN.PAS` constructs a dark board through the reconstructed Pascal world and board routines, starts the player at `(5,5)`, places a Torch at `(6,5)`, an Energizer at `(7,5)`, and a solid tile at `(8,5)`, and sets a ten-second board time limit. The normalized `RTRN` tape drives movement onto the Torch, torch activation, movement onto the Energizer, and a neutral input. The actual game loop then continues through 120 cycles so the Energizer expires, the board timer advances, and the player dies. The replay also observes a dark-area redraw at zero-based capture coordinate `(14,4)` as CP437 shade `0xB0`, color attribute `0x07`.

The Pascal capture invokes the real `GamePlayLoop`, `BoardDrawTile`, torch and Energizer callbacks, timer processing, damage/death behavior, `WorldSave`, and `WorldLoad`. Production invokes shared `zoo_tick`, the element callbacks, and the production serializer/loader. The test adapter records output and normalizes environmental input and time; it does not replace the gameplay routines.

## Evidence and reproduction

Evidence classes: **D** (direct compiled Pascal), **DP** (direct Pascal-to-production differential), and **I** (integrated actual-game-loop replay).

The clean runner command was:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain build/pascal-compat-run/Z -DosboxExecutable "<external DOSBox-X 2026.10.01 executable>"
```

It exited 0 at local source revision `1c39252ba592d90760fdab1acc2a37c46fa0307f`, with **120/120 direct Pascal sessions**, **50/50 configured Pascal-to-production comparisons**, and the diagnostic first-divergence negative control passing. DOSBox-X was version 2026.10.01, external executable SHA-256 `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`. Borland Turbo Pascal 5.5, DOSBox-X, Node.js 22.19.0, and Python 3.13.7 are external verification tools; proprietary toolchain binaries are not included.

The fresh oracle capture hashes are:

| Output | Size | SHA-256 |
|---|---:|---|
| `INTEGPR.OUT` | 927,772 | `b338a11b65610bd878aa95c300ae547fb974a12a74ec58e255c1deab98488f5c` |
| `INTEGSND.OUT` | 830 | `4297c0005e505cf22cc31af6be74f8874f957e087808f32ef5e2157a63d1ebb9` |
| `INTEGCEL.OUT` | 49,483 | `f8a80659f8fa47f05e35056b695a3fea09239d8d2290be49bc312e722a1a21d4` |
| `INTEGSAV.SAV` | 878 | `4c0e967163621b0a0ee7784b903674ec277219d37ca8738e404e328300c6898f` |
| `INTEGLOD.OUT` | 3 | `f1b2f662800122bed0ff255693df89c4487fbdcf453d3524a42d4ec20c3d9c04` |
| `INTEG2SV.SAV` | 878 | `4c0e967163621b0a0ee7784b903674ec277219d37ca8738e404e328300c6898f` |

## Controlled setup and instrumentation

- Seed `RandSeed=0` and call `GenerateTransitionTable` before `WorldCreate`, matching ZZT startup order. Set `World.Info.unk1=0` because `WorldCreate` leaves that word undefined and the production fixture uses initialized storage.
- Set the fixture's player health, dark-board flag, time limit, board start position, Torch/Energizer/solid tiles, and zero tick speed explicitly.
- Seed the otherwise undefined `pauseBlink` local through the existing staged probe hook.
- Use a normalized `RTRN` input tape. In the staged Pascal input adapter, `T` records the Pascal key byte; the production adapter preserves the same snapshot key byte while translating that normalized command to LibZoo's mapped torch action.
- Disable physical sound output while capturing the real Pascal `SoundQueue` requests and post-call queue state. No PC-speaker output is compared.
- Disable wall-clock reads for this fixture. A verification-only hook advances Pascal `TimerTicks` by two PIT ticks after each completed cycle; the production replay uses its deterministic cycle clock. This makes timer transitions comparable without asserting physical scheduler or host-pointer identity.
- Compile and execute the Pascal replay in separate fresh DOSBox-X sessions. Capture `VideoWriteText`, sound events, effective cell changes, and serialized outputs while keeping the original game loop and callbacks in control.
- Reference source remains untouched. The clean source hashes are recorded in [`reference_source_hashes.sha256`](../reference_source_hashes.sha256); all test hooks are explicit in [`verify.ps1`](../../pascal-compat/verify.ps1) and the fixture is [`DARKIN.PAS`](../../pascal-compat/DARKIN.PAS).

## Source and implementation locations

- Reconstructed source: [`GAME.PAS`](../../../third_party/reconstruction-of-zzt/SRC/GAME.PAS) (`GamePlayLoop`, `BoardDrawTile`), [`ELEMENTS.PAS`](../../../third_party/reconstruction-of-zzt/SRC/ELEMENTS.PAS) (Torch, Energizer, damage/death callbacks).
- Production: [`zoo_game.c`](../../../third_party/libzoo/src/libzoo/zoo_game.c) (`zoo_tick`), [`zoo_elements.c`](../../../third_party/libzoo/src/libzoo/zoo_elements.c) (element callbacks).
- Direct Pascal fixture: [`DARKIN.PAS`](../../pascal-compat/DARKIN.PAS).
- Production replay harness: [`oop_movement.c`](../../runtime-compat/oop_movement.c).
- Differential and divergence diagnostics: [`compare-integrated-gameplay.js`](../../pascal-compat/compare-integrated-gameplay.js), scenario `dark-room-torch-energizer-timers-death`.
- Clean runner: [`verify.ps1`](../../pascal-compat/verify.ps1).
- Frozen fixture record: [`integrated_replay_fixture_manifest.json`](../integrated_replay_fixture_manifest.json), ID `E2-10-F06`.
