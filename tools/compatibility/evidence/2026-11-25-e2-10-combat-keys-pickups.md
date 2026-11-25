# E2-10-F03: combat, bullets, keys/doors, and pickups

## Result

**Passed as the third required E2-10 integrated fixture.** The fixed corpus is 3/10 complete; F04–F10 remain pending and E2-10 remains open.

`COMBATIN.PAS` creates a controlled board and runs the reconstructed original `GamePlayLoop` for six zero-speed cycles with normalized input tape `RFRRRRR`. The production side enters the shared LibZoo `zoo_tick` loop with the same state and tape. The comparator checks named state snapshots, logical sound events, ordered playfield writes, and a fresh-world save/load/round trip. Gameplay routines are not replaced with a behavioral model.

| Evidence | Result |
|---|---:|
| Named checkpoints | 12 (`L0`, `S0`, `T1`, `C1`–`C6`, `T2`, `T3`, `F0`) |
| State fields | 41,122 compared; 0 differences |
| Fields by checkpoint | `[3416, 3416, 3455, 3455, 3424, 3424, 3424, 3430, 3430, 3416, 3416, 3416]` |
| Logical sound queue | 7 events / 186 fields; 0 differences |
| Ordered playfield writes | 664 events / 2,656 fields; 0 differences |
| Fresh-world `.SAV` | 884 bytes; 558 stable bytes matched; 326 undefined/pointer positions classified; 0 stable-byte differences |
| Same-file load and second save | Pascal `WorldLoad=true`; production `zoo_world_load=0`; 876 preserved bytes matched; 8 process-pointer positions classified |
| Unexplained fixture mismatches | 0 |

The assertions confirm that the player collects the red key, fires and removes the Lion, opens the matching red door, collects the Gem and Ammo, and ends at `(10,5)`. Final state is ammo 5, gems 1, health 101, score 10, the red key cleared, and one remaining stat (the player). The matching door and the Lion, key, Gem, and Ammo tiles are consumed. The production assertions also check these final values independently of the snapshot comparison.

## Reproduction

From the repository root with the external Turbo Pascal 5.5 and DOSBox-X dependencies configured:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/runtime-compat/verify.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain build/pascal-compat-run/Z -DosboxExecutable <DOSBox-X executable>
```

The clean Pascal runner exited 0: 87/87 direct Turbo Pascal sessions and 47/47 configured Pascal-to-production comparisons passed. It ran F01–F03 and the integrated diagnostic negative control. The production runtime verifier also exited 0 after the normalized-input harness cleanup. No unexplained mismatch was reported.

## Controlled inputs and instrumentation

- `RandSeed` is zero before `WorldCreate`; `World.Info.unk1` is set to zero because `WorldCreate` leaves that word undefined and the production fixture uses zero-initialized state.
- `TickSpeed` is zero. The staged `SoundProbeSetSchedule(0, 0, 0)` supplies the deterministic logical clock tape.
- A temporary normalized-input hook is inserted into a staging copy of `INPUT.PAS` only for the `COMBATIN` compile. The tape supplies right, fire, then right inputs. The normalizer is removed and the original staged `INPUT.PAS` restored before other probes, preserving their static memory and allocation boundaries.
- The original `GamePlayLoop`, element callbacks, projectile processing, and the production `zoo_tick` path remain active. `VideoWriteText` and `SoundQueue` are observed at their logical engine boundaries.
- The Pascal program is compiled and executed in separate DOSBox-X sessions to control compiler-inherited DOS allocation history.
- Only undefined Pascal padding/string tails and process-specific stat pointer bytes are classified in save comparisons.

## Source and fixture locations

- Reconstructed routines: [`GAME.PAS`](../../../third_party/reconstruction-of-zzt/SRC/GAME.PAS) (`GamePlayLoop`) and [`ELEMENTS.PAS`](../../../third_party/reconstruction-of-zzt/SRC/ELEMENTS.PAS) (player, projectile, key, door, and pickup callbacks). Original source hashes are recorded in [`reference_source_hashes.sha256`](../reference_source_hashes.sha256).
- Direct Pascal replay: [`COMBATIN.PAS`](../../pascal-compat/COMBATIN.PAS).
- Production replay: [`oop_movement.c`](../../runtime-compat/oop_movement.c), `run_integrated_combat_key_pickup_checks`; cycle processing uses shared `zoo_tick` and callbacks in `zoo_elements.c`.
- Differential capture: [`compare-integrated-gameplay.js`](../../pascal-compat/compare-integrated-gameplay.js), scenario `combat-keys-pickups`.
- Clean runner: [`verify.ps1`](../../pascal-compat/verify.ps1).
- Required scenario record: [`integrated_replay_fixture_manifest.json`](../integrated_replay_fixture_manifest.json), ID `E2-10-F03`.

## Oracle output hashes

Outputs are reproducible files in the ignored `build/pascal-compat-verification` staging directory and are not committed as generated binaries.

| Output | Size | SHA-256 |
|---|---:|---|
| `INTEGPR.OUT` | 86,664 | `42c5650df7c79f44a5c77e8b78f8497da0b29e4755a9f73994836edf3273860f` |
| `INTEGSND.OUT` | 468 | `360c102739a77de238ef1855f75d08e3b163e0d8a459c8fcee4b2dd59e30b61c` |
| `INTEGCEL.OUT` | 8,829 | `90ffd422fa7cd22299920fb2579ee81f9e9b3c757596f07d43ad7ecf9d841543` |
| `INTEGSAV.SAV` | 884 | `3185f2f7fa6e742512fe403fc1347ba9f4d912596a1ff0c54573ccfed8b97b27` |
| `INTEGLOD.OUT` | 3 | `f1b2f662800122bed0ff255693df89c4487fbdcf453d3524a42d4ec20c3d9c04` |
| `INTEG2SV.SAV` | 884 | `3185f2f7fa6e742512fe403fc1347ba9f4d912596a1ff0c54573ccfed8b97b27` |

## Verification record

- Date: 2026-11-25.
- Tested engine/test source revision: `ae4b50128dcffe9d414ce49570491915a1106f4c`.
- LibZoo baseline: `7fb3e89a8467e66a4b8cbc290c708b9fa5290886`; local LibZoo overlay: `e5d2fe7`.
- Compiler: Borland Turbo Pascal 5.5 (`TPC.EXE` and `TURBO.TPL`, external and not redistributed).
- DOS environment: DOSBox-X 2026.10.01, external executable SHA-256 `072c1944635ccc545a816e128625e3405feab559021e4ef60b2d512f97f754c0`.
- Node.js: v22.19.0; Python: 3.13.7.
- `tools/runtime-compat/verify.ps1` and the complete `tools/pascal-compat/verify.ps1` runner exited 0.
- E2-10 is still partial. Remaining fixtures and other engine blockers are listed in [`COMPATIBILITY_PROGRESS.md`](../../../COMPATIBILITY_PROGRESS.md).
