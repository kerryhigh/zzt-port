# E2-10-F01: DEMO start and ordinary traversal

## Result

**Passed as the first required E2-10 integrated fixture.** This does not close E2-10: the frozen corpus is 1/10 complete and fixtures F02–F10 remain pending.

The direct Pascal probe loaded the checked-in DEMO world, changed to its saved current board, entered that board, and ran the original paused `GamePlayLoop` through six zero-speed cycles. The production comparator ran the corresponding shared LibZoo world-load, board-change, board-entry, game-start, and `zoo_tick` paths. No gameplay routine was replaced by a behavioral model.

| Evidence | Result |
|---|---:|
| Named checkpoints | 12 (`L0`, `S0`, `T1`, `C1`–`C6`, `T2`, `T3`, `F0`) |
| State fields | 41,472 compared; 0 differences |
| Player traversal | Board 1: (30,12) → (31,12) → (32,12) |
| Ordered playfield writes | 5 events / 20 fields; 0 differences |
| Logical sound queue | 0 events; no overflow |
| DEMO-derived `.SAV` | 26,465 bytes; 25,241 stable bytes matched; 1,224 process-specific pointer positions classified; 0 classified differences |
| Same-file load and second save | Pascal `WorldLoad=true`; production `zoo_world_load=0`; second save SHA-256 equals first save |
| Unexplained fixture mismatches | 0 |

The five ordered cell writes and player movement confirm that the replay passed through actual play rather than only comparing a loaded world. The six-byte right-direction input tape exercises the original Pascal input-buffer retention behavior: the tested session moved twice and then continued through the remaining cycles.

## Reproduction

From the repository root, with the external Turbo Pascal 5.5 and DOSBox-X dependencies configured:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/runtime-compat/verify.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain build/pascal-compat-run/Z -DosboxExecutable <DOSBox-X executable>
```

The clean run exited 0. It completed 85/85 direct compiled-Pascal sessions and 45/45 configured Pascal-to-production differential invocations. The integrated DEMO Pascal executable is compiled in one DOSBox-X session and run in a fresh session to control compiler-inherited DOS allocation history. The run also verifies the earlier synthetic `INTEGPRB` prototype, which remains separate from required fixture credit.

## Controlled inputs and instrumentation

- `DEMO.ZZT` is loaded without mutation. Its staged bytes are 26,465 bytes, SHA-256 `b54610b5a52b125fa3083f38f6beec1cf2178655575fc0d60e999e2659253831`.
- `RandSeed` is set to zero before `WorldCreate` and `WorldLoad` to make subsequent RNG use reproducible.
- `TickSpeed` is set to zero. The staged `SoundProbeSetSchedule(0, 0, 0)` supplies a deterministic logical clock tape so each scheduled zero-speed cycle is due without relying on DOSBox wall-clock scheduling.
- The input tape `666666` is queued before the original `GamePlayLoop`. The Pascal `InputUpdate` implementation remains intact; its key-buffer quirk means the six queued bytes produce two moves in this replay.
- `VideoWriteText` and `SoundQueue` are observed by test-only captures. The engine's state, callbacks, and tick loop remain the production/oracle paths.
- The comparison masks only stat data-pointer bytes that differ with process addresses. Every other byte of the loaded-file save and round-trip save is compared.

## Source and fixture locations

- Reconstructed reference routines: [`GAME.PAS`](../../../third_party/reconstruction-of-zzt/SRC/GAME.PAS) (`WorldLoad`, `BoardChange`, `BoardEnter`, `GamePlayLoop`). The preserved reference hash for this source is in [`reference_source_hashes.sha256`](../reference_source_hashes.sha256): `38ffcd9a3fd10af6ebfa7c457d957a3737bbdde69a081e3b3c3c7043aba09596`.
- Production routines: `third_party/libzoo/src/libzoo/zoo_game_io.c` (`zoo_world_load`, `zoo_world_save`); `third_party/libzoo/src/libzoo/zoo_game.c` (`zoo_board_change`, `zoo_board_enter`, `zoo_game_start`, `zoo_tick`).
- Input tape and DOS oracle: [`DEMOINT.PAS`](../../pascal-compat/DEMOINT.PAS).
- Production fixture harness: [`oop_movement.c`](../../runtime-compat/oop_movement.c), `run_integrated_demo_gameplay_checks`.
- Comparator: [`compare-integrated-gameplay.js`](../../pascal-compat/compare-integrated-gameplay.js).
- Required scenario record: [`integrated_replay_fixture_manifest.json`](../integrated_replay_fixture_manifest.json), ID `E2-10-F01`.
- Input world: `third_party/reconstruction-of-zzt/RES/DEMO.ZZT` (already tracked in the project).

## Oracle output hashes

Files are reproducible outputs in the ignored `build/pascal-compat-verification` staging directory; they are not included as generated binaries in the repository.

| Output | Size | SHA-256 |
|---|---:|---|
| `INTEGPR.OUT` | 102,882 | `d35403a5780d86757f5044cd0e60adb51ff3dea492241fce9431c3ac380f502d` |
| `INTEGSND.OUT` | 5 | `043b5ae7334040ef63298adca08912d5287ad37e66f9425316adc50d2c1ab652` |
| `INTEGCEL.OUT` | 67 | `52b98df54a92143b1816983a277e902ab8b02ff567c4adfe40ac1ef319ddaa40` |
| `INTEGSAV.SAV` | 26,465 | `d6be70b9d00bbc302b43e5b045c979821d0d5fafb04c68a5152bc2cfbef5f0e0` |
| `INTEG2SV.SAV` | 26,465 | `d6be70b9d00bbc302b43e5b045c979821d0d5fafb04c68a5152bc2cfbef5f0e0` |

## Verification record

- Date: 2026-11-25.
- Tested local revision: `f3b0f2ff6afdd80c7e8980ebf6930ae9cbd8672d` (integrated replay implementation).
- LibZoo baseline: `7fb3e89a8467e66a4b8cbc290c708b9fa5290886`; current local LibZoo overlay at verification: `e5d2fe7`.
- Pascal compiler: Borland Turbo Pascal 5.5 (`TPC.EXE` / `TURBO.TPL`, external and not redistributed).
- DOS environment: DOSBox-X 2026.10.01, external executable SHA-256 `072c1944635ccc545a816e128625e3405feab559021e4ef60b2d512f97f754c0`.
- Node.js: v22.19.0. Python: 3.13.7.
- Full direct Pascal runner command exited 0; the source reference was instrumented only in the clean staging copy.
- The production integrated WebAssembly harness compiled and `tools/runtime-compat/verify.ps1` exited 0.
- Full-run unexplained mismatch count: 0. The other Engine 2.0 blockers remain open according to [`COMPATIBILITY_PROGRESS.md`](../../../COMPATIBILITY_PROGRESS.md).
