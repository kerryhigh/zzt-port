# E2-10-F04: object stat order and interacting OOP scripts

## Result

**Passed as the fourth required E2-10 integrated fixture.** The frozen corpus is now 4/10 complete; F05–F10 remain open, and E2-10 remains a release blocker.

`STATORD.PAS` creates two scripted Objects in stat order 1 then 2, with their board positions deliberately ordered in the opposite direction: stat 1 at `(20,5)` and stat 2 at `(10,5)`. In one execution of the original `GamePlayLoop`, stat 1 runs `#SET FIRST` and `#GIVE GEMS 1`; stat 2 then runs `#IF FIRST #GIVE AMMO 1`. The C replay uses shared LibZoo `zoo_tick` and the same script bytes and ordering. This checks that execution follows stat order and that the flag set by the earlier Object is visible to the later Object in that script cycle.

| Evidence | Result |
|---|---:|
| Named checkpoints | 12 (`L0`, `S0`, `T1`, `C1`–`C6`, `T2`, `T3`, `F0`) |
| State fields | 41,904 compared; 0 differences |
| Fields per checkpoint | 3,492 at each checkpoint |
| Logical sound queue | 0 events |
| Ordered playfield writes | 5 events / 20 fields; 0 differences |
| Fresh-world `.SAV` | 974 bytes; 598 stable bytes matched; 376 undefined/pointer positions classified, including 6 process-pointer byte differences; 0 stable-byte differences |
| Same-file load and second save | Pascal `WorldLoad=true`; production `zoo_world_load=0`; 962 preserved bytes matched; 12 process-pointer positions classified |
| Unexplained fixture mismatches | 0 |

The direct Pascal trace shows `Board.StatCount=2` (the player is stat 0 and excluded from this count). The first script-active cycle is C2; at that checkpoint ammo and gems are both 1, and they remain 1 in the final state. The stat positions and order are preserved. The comparator checks all cells and sentinels, stat/script state, scheduler/RNG state, captured sound queue, ordered cell writes, and serialized save/load output.

## Reproduction

From the repository root with external Turbo Pascal 5.5 and DOSBox-X configured:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/runtime-compat/verify.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain build/pascal-compat-run/Z -DosboxExecutable <DOSBox-X executable>
```

The clean Pascal runner exited 0: 88/88 direct Turbo Pascal sessions and 48/48 configured Pascal-to-production comparisons passed. The runtime compatibility verifier exited 0. F01–F04 passed; the full Engine 2.0 milestone remains open.

## Controlled inputs and instrumentation

- Set `RandSeed` to zero before `WorldCreate` and set `World.Info.unk1` to zero because `WorldCreate` leaves the latter word undefined while the production harness begins with zero-initialized state.
- Build the player and two Objects using the original `BoardCreate` and Pascal `AddStat` calls. The Object color argument is explicitly `$0A`, matching `STATORD.PAS`.
- Give the Objects the source scripts using Pascal `GetMem`/`Move`; use zero tick speed and the staged logical clock schedule, and queue the normalized key bytes with `InputProbeSetKeyBuffer('66')` before entering `GamePlayLoop`.
- Capture video-cell writes and sound-queue calls at their boundaries. The original Pascal game loop, element callbacks, and production `zoo_tick` remain active; no gameplay routine is replaced by a behavioral model.
- Compile and run the probe in separate DOSBox-X sessions to control compiler-inherited allocation history. Production fixture scripts are stored in LibZoo's modeled board-data heap so their state and serialized pointer locations follow the engine's heap representation.
- Compare process-specific Pascal pointers and undefined padding only under the existing nondeterminism contract; stable serialized bytes must still match exactly.

During fixture bring-up, the first mismatch exposed two harness assumptions rather than a production differential: the C setup used LibZoo's `0xFF` choice-color sentinel instead of Pascal's explicit `$0A`, and a test assertion counted the player despite Pascal `Board.StatCount` excluding stat 0. The first script-active state is C2, not the loop-start C1 checkpoint. Those assertions now follow the compiled Pascal trace. Serialized-byte checks were also moved after checkpoint/event comparisons so a final file difference cannot obscure an earlier gameplay divergence. No Pascal gameplay semantics or oracle bytes were modified to resolve these fixture errors.

## Source and fixture locations

- Reconstructed `GamePlayLoop` and `AddStat`: [`GAME.PAS`](../../../third_party/reconstruction-of-zzt/SRC/GAME.PAS).
- Direct compiled-Pascal replay: [`STATORD.PAS`](../../pascal-compat/STATORD.PAS).
- Production replay setup: [`oop_movement.c`](../../runtime-compat/oop_movement.c), `run_integrated_script_stat_order_checks`; gameplay uses shared `zoo_tick` and `zoo_e_object_tick`.
- Comparator and first-divergence reporting: [`compare-integrated-gameplay.js`](../../pascal-compat/compare-integrated-gameplay.js), scenario `object-stat-order`.
- Full runner: [`verify.ps1`](../../pascal-compat/verify.ps1).
- Required fixture record: [`integrated_replay_fixture_manifest.json`](../integrated_replay_fixture_manifest.json), ID `E2-10-F04`.
- Untouched Pascal reference source hashes: [`reference_source_hashes.sha256`](../reference_source_hashes.sha256).

## Oracle output hashes

Generated outputs are in the ignored `build/pascal-compat-verification` directory and are not committed as build artifacts.

| Output | Size | SHA-256 |
|---|---:|---|
| `INTEGPR.OUT` | 88,918 | `7e173faf48e75472ff4b07a6caefd12037437001fbaabebb26847e4a41cb0bc3` |
| `INTEGSND.OUT` | 5 | `043b5ae7334040ef63298adca08912d5287ad37e66f9425316adc50d2c1ab652` |
| `INTEGCEL.OUT` | 57 | `4b25266c6a292c445c6b5a850eccc9a5c8928eadc9c1d84a41624d9ba289d82a` |
| `INTEGSAV.SAV` | 974 | `16e9ab5cf42ddf0c72d68ecbd05f25fd034155199c55a5a8c6f7294e053fb3b1` |
| `INTEGLOD.OUT` | 3 | `f1b2f662800122bed0ff255693df89c4487fbdcf453d3524a42d4ec20c3d9c04` |
| `INTEG2SV.SAV` | 974 | `8bb39b5208775a2efe2426ad74a2a3d8590adff3e8d41dd5f632de51fb599140` |

## Verification record

- Date: 2026-11-25.
- Tested source revision: `ca271c7be437f6b7adf205b2c4a984055ef1814c`.
- LibZoo upstream baseline: `7fb3e89a8467e66a4b8cbc290c708b9fa5290886`; local overlay: `e5d2fe7`.
- Compiler: Borland Turbo Pascal 5.5 (`TPC.EXE` and `TURBO.TPL`, external and not redistributed).
- DOS environment: DOSBox-X 2026.10.01; external executable SHA-256 `072c1944635ccc545a816e128625e3405feab559021e4ef60b2d512f97f754c0`.
- Node.js: v22.19.0; Python: 3.13.7.
- Commands: `tools/runtime-compat/verify.ps1` exit 0; `tools/pascal-compat/verify.ps1 -PascalToolchain build/pascal-compat-run/Z -DosboxExecutable <external DOSBox-X>` exit 0.
- The complete clean run passed 88 direct sessions and 48/48 configured comparisons. F04 is complete; E2-10 remains partial at 4/10 fixtures.
