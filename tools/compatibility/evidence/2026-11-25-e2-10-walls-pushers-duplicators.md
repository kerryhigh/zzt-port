# E2-10-F02: walls, pushers, duplicators, and collision ordering

## Result

**Passed as the second required E2-10 integrated fixture.** E2-10 remains open: the fixed ten-fixture corpus is 2/10 complete, with F03–F10 pending.

The direct Turbo Pascal probe creates a deterministic board, then runs the reconstructed original `GamePlayLoop` through six zero-speed cycles. The production side constructs the corresponding state and enters the shared LibZoo `zoo_tick` loop. The comparator checks named snapshots, sound queue events, ordered logical cell writes, and fresh-world plus loaded-file save behavior. No gameplay routine is replaced by a behavioral model.

| Evidence | Result |
|---|---:|
| Named checkpoints | 12 (`L0`, `S0`, `T1`, `C1`–`C6`, `T2`, `T3`, `F0`) |
| State fields | 42,378 compared; 0 differences |
| Fields by checkpoint | `[3498, 3498, 3498, 3498, 3552, 3552, 3552, 3552, 3552, 3542, 3542, 3542]` |
| Logical sound queue | 4 events / 56 fields; 0 differences |
| Ordered playfield writes | 16 events / 64 fields; 0 differences |
| Fresh-world `.SAV` | 1,075 bytes; 643 stable bytes matched; 432 classified uninitialized/pointer positions; 0 stable-byte differences |
| Same-file load and second save | Pascal `WorldLoad=true`; production `zoo_world_load=0`; 1,051 preserved bytes matched; 24 process-pointer positions classified |
| Unexplained fixture mismatches | 0 |

The assertions confirm a player pushes a Boulder into a Solid, a two-pusher chain moves a Boulder until blocked by a Solid, and a Duplicator creates an object whose copied `#GIVE GEMS 1` script runs during the same tick. The final state has the player at x=7, the pusher chain at x=11/x=12, the blocked Boulder at x=13, the blocking Solid at x=14, and five stats including the duplicated object. The source and clone scripts both execute.

## Reproduction

From the repository root, with the external Turbo Pascal 5.5 and DOSBox-X dependencies configured:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/runtime-compat/verify.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain build/pascal-compat-run/Z -DosboxExecutable <DOSBox-X executable>
```

The clean full run exited 0: 86/86 direct compiled-Pascal sessions and 46/46 configured Pascal-to-production comparisons passed. The run also passed both required integrated fixtures (F01 DEMO traversal and F02 movement/collision), runtime compatibility, and the integrated diagnostic negative control. The full runner reported zero unexplained compatibility mismatches.

## Controlled inputs and instrumentation

- `RandSeed` is set to zero before constructing the board.
- `World.Info.unk1` is set to zero because `WorldCreate` leaves it undefined while the production fixture uses zero-initialized storage. This controls an environment-dependent value and does not change gameplay routines.
- Six right-direction bytes (`666666`) are supplied to the original input buffer. `TickSpeed` is zero and the staged `SoundProbeSetSchedule(0, 0, 0)` provides a deterministic logical clock tape.
- `VideoWriteText` cell writes and `SoundQueue` requests are captured at their logical engine boundaries. The original callbacks and `GamePlayLoop` remain intact.
- The Pascal executable is compiled in a separate DOSBox-X session and run in a fresh session, controlling compiler-inherited DOS allocation history.
- Only uninitialized Pascal payload tails and process-specific pointer bytes are classified in save comparisons. The generated save has six differences among 432 classified positions; stable bytes match exactly. The loaded-file round trip compares all 1,051 stable bytes exactly and classifies 24 stat-pointer positions.

## Source and fixture locations

- Reconstructed routines: [`GAME.PAS`](../../../third_party/reconstruction-of-zzt/SRC/GAME.PAS) (`GamePlayLoop`) and [`ELEMENTS.PAS`](../../../third_party/reconstruction-of-zzt/SRC/ELEMENTS.PAS) (pusher, Duplicator, collision, and object behavior). Their preserved hashes are recorded in [`reference_source_hashes.sha256`](../reference_source_hashes.sha256).
- Direct Pascal replay: [`MOTIONIN.PAS`](../../pascal-compat/MOTIONIN.PAS).
- Production gameplay setup: [`oop_movement.c`](../../runtime-compat/oop_movement.c), `run_integrated_wall_pusher_duplicator_checks`; tick processing uses shared `zoo_tick`.
- Differential capture: [`compare-integrated-gameplay.js`](../../pascal-compat/compare-integrated-gameplay.js), scenario `walls-pushers-duplicators`.
- Clean runner: [`verify.ps1`](../../pascal-compat/verify.ps1).
- Required scenario record: [`integrated_replay_fixture_manifest.json`](../integrated_replay_fixture_manifest.json), ID `E2-10-F02`.

## Oracle output hashes

Outputs live in the ignored `build/pascal-compat-verification` staging directory and are not committed as generated files.

| Output | Size | SHA-256 |
|---|---:|---|
| `INTEGPR.OUT` | 89,988 | `568e316ca66dcc576273ca16b9e205573e85e25c26abde36adcdfb31fb97d030` |
| `INTEGSND.OUT` | 143 | `be4a90ceb8eadeb4cfd756d0f13d1571f2456e38dec128ccbd9507e0e0735bad` |
| `INTEGCEL.OUT` | 191 | `0e8aa57f4182ccc42341654394f6ae0fcad6950f1472be9c0ff436bbf216382b` |
| `INTEGSAV.SAV` | 1,075 | `558e73d4843324a1b48ac11633761d1c6e4af87bd3c56b16551662607c6e5bc2` |
| `INTEGLOD.OUT` | 3 | `f1b2f662800122bed0ff255693df89c4487fbdcf453d3524a42d4ec20c3d9c04` |
| `INTEG2SV.SAV` | 1,075 | `16e6d2e68a2695b8ab8c5a61398330cdb8609b5607d161c19e31732d7cbdadc5` |

## Verification record

- Date: 2026-11-25.
- Tested local source revision: `8d4e5a66018ce5f0762f4e5cd509d871fb10d6b3`.
- LibZoo baseline: `7fb3e89a8467e66a4b8cbc290c708b9fa5290886`; local LibZoo overlay: `e5d2fe7`.
- Compiler: Borland Turbo Pascal 5.5 (`TPC.EXE` and `TURBO.TPL`, external and not redistributed).
- DOS environment: DOSBox-X 2026.10.01, external executable SHA-256 `072c1944635ccc545a816e128625e3405feab559021e4ef60b2d512f97f754c0`.
- Node.js: v22.19.0; Python: 3.13.7.
- `tools/runtime-compat/verify.ps1` and the complete `tools/pascal-compat/verify.ps1` runner exited 0.
- E2-10 is still partial. Remaining engine blockers and open questions are listed in [`COMPATIBILITY_PROGRESS.md`](../../../COMPATIBILITY_PROGRESS.md).
