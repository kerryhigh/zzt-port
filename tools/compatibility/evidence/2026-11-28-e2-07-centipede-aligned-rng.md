# E2-07 AC003: aligned-axis Centipede RNG callers

## Result

Both existing aligned-axis `Random(10)` call sites in `ElementCentipedeHeadTick` now have direct compiled-Pascal and production differential evidence:

- `E2-07-RNG-ELEMENTS-0205`, `ELEMENTS.PAS:205`: X-aligned player path.
- `E2-07-RNG-ELEMENTS-0208`, `ELEMENTS.PAS:208`: Y-aligned player path after the first condition short-circuits.

The direct comparator matched 2 cases and 34/34 numeric fields. Both consume one `Random(10)` draw of value 0 from seed 0, finish with seed words low/high `1/0`, and match the Centipede stat position/direction plus source and destination tile element/color. The first path moves the head from `(10,10)` to `(10,9)`; the second moves it to `(9,10)`. AC003 remains partial; this evidence adds no new acceptance cases.

## Reproduction

The full clean verifier command was:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain 'build/pascal-compat-run/Z' -DosboxExecutable '<external DOSBox-X 2026.10.01 executable>'
```

The run exited 0 with 154 direct Turbo Pascal sessions (57 standard and 97 standalone), 79/79 configured Pascal-to-production comparisons, and E2-10 fixtures F01–F10 passing at 238 checkpoints, 824,821 state fields, and 169 logical cycles. The focused Centipede comparator passed as part of that runner. The broader suite reported zero unexplained comparable mismatches. Android and Windows builds were not rerun in this checkpoint.

Toolchain: Borland Turbo Pascal 5.5 (`TPC.EXE` and `TURBO.TPL`, external and excluded from the repository), DOSBox-X 2026.10.01, and Node.js v22.19.0. The absolute DOSBox-X path is omitted from this public evidence record.

## Oracle output and provenance

- Probe: `tools/pascal-compat/CENTIRNG.PAS`.
- Output: `build/pascal-compat-verification/CENTIRNG.OUT`, 106 bytes, SHA-256 `C2C32868915988A0BD834E86FF690C5F38309B74CA453A15B19043A60921EC25`.
- Comparator: `tools/pascal-compat/compare-centipede-rng.js`.
- Production capture: `tools/runtime-compat/oop_movement.c::run_centipede_rng_caller_checks`, invoking the shared `zoo_e_centipede_head_tick` callback and `zoo_random_next` through the deterministic test random hook.
- Reconstructed oracle source: `third_party/reconstruction-of-zzt/SRC/ELEMENTS.PAS`, SHA-256 `9850C7C2F3B8BFA16B2D4842BB85B1FE5100630C3C7D3C73C5007EBC095C4F71`.
- Tested local source commit: `9354bbb831a7e667c2cb053d5324f7280219def1`; tree `68fadd96b618fff08974512131986913face247b`.
- Published source commit: `d7aa1239c1898ae0a78bd92e9240a38d5c4cf359` (same tree).

The output rows are:

```text
x-aligned 0 0 1 10 0 1 0 10 9 0 -1 0 112 10 9 44 15
y-aligned 0 0 1 10 0 1 0 9 10 -1 0 0 112 9 10 44 15
```

For each row, compared values are initial RandSeed low/high; draw count, range, and result; final seed low/high; head stat position and direction; source head tile element/color; destination coordinates and tile element/color.

## Oracle interventions

The Turbo Pascal probe allocates `IoTmpBuf` before `WorldCreate`, installs a quiet `VideoWriteText` callback to suppress rendering, sets `TickSpeed=4`, creates explicit player and Centipede head stats/tiles, initializes the relevant `Under`, `P1`, `P2`, direction, leader/follower, and energizer fields, and sets `RandSeed=0` for each case. It previews `Random(10)` and restores the original seed before invoking the original `ElementDefs[E_CENTIPEDE_HEAD].TickProc(1)`; the preview does not change the reference routine's RNG state. The direct probe does not modify Pascal gameplay source, the callback, or RNG semantics.

The production WASM capture initializes the equivalent board and seed independently for each case, records the requested random range and value, and calls the shared Centipede tick callback. It does not substitute a separate Centipede behavioral model. The comparator checks the 17 captured numeric fields in each case.

## Remaining E2-07 AC003 work

Direct caller evidence covers 21 of the 29 frozen RNG call sites. Eight still lack direct D/DP evidence; two of those have integrated-only F06/F07 support counted under E2-10. `Randomize` time seeding, four sound drum assignments, the remaining caller cases, and the unresolved `GAME.PAS:1699` register-message path remain open. The complete finite status is recorded in `tools/compatibility/acceptance_inventory.json` and generated in `COMPATIBILITY_PROGRESS.md`.
