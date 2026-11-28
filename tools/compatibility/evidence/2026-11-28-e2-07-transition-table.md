# E2-07 AC003: `GenerateTransitionTable` caller

## Result

The untouched Turbo Pascal 5.5 `Game.GenerateTransitionTable` routine and the production initializer matched all **1,500 shuffled 60×25 coordinates** and both 16-bit words of the resulting `RandSeed`. This directly verifies the `Random(TransitionTableSize)` caller at `GAME.PAS:140`, including its 1,500 draws, order, coordinate swaps, and final RNG state.

The controlled seed was zero. The final seed words were low `18372`, high `9231`. The captured `TRNTBL.OUT` was 16,145 bytes with SHA-256 `35a9c454576a2a70825693cb4549bb5531a83191d5c7de738378b1316ce81d7f`. The direct Pascal/production comparison checked **3,002/3,002 numeric fields** with zero differences.

## Reproduction

Run the clean direct Pascal and production suite from the repository root with the external Turbo Pascal 5.5 files and DOSBox-X 2026.10.01 configured:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 `
  -PascalToolchain 'build/pascal-compat-run/Z' `
  -DosboxExecutable '<external DOSBox-X executable>'
```

The probe is `tools/pascal-compat/TRNTBL.PAS`; `tools/pascal-compat/compare-transition-table.js` reports the first differing table slot with neighboring coordinates or the mismatching seed word. The production capture is `run_transition_table_capture_checks` in `tools/runtime-compat/oop_movement.c`, exported by `tools/runtime-compat/verify.ps1` from `integrated_gameplay.wasm`.

## Oracle boundary

The reference source is the reconstructed Pascal tree, with `GAME.PAS` SHA-256 `38ffcd9a3fd10af6ebfa7c457d957a3737bbdde69a081e3b3c3c7043aba09596` and `GAMEVARS.PAS` SHA-256 `95f0f75261840f15297465e23a40f7c7df592292c810a53a4faa144ac318e1e0`. No Pascal gameplay source was instrumented or modified. The probe sets `RandSeed := 0`, calls the original routine, and writes its resulting table and seed; it does not call `WorldCreate` or consume any other random values.

The production capture calls `zoo_transition_table_init`, the same helper now called by `zoo_state_init`; it does not duplicate the shuffle model. Refactoring the existing startup loop into that static helper preserves its operation order and draw count. The harness only zeros a state object, assigns the controlled seed, invokes the production helper, and captures coordinates and seed words.

## Evidence and remaining scope

- Evidence classes: direct compiled Pascal **D**, direct Pascal/production differential **DP**, source mapping **S**; existing integrated F06 evidence remains **I** and is still credited only under E2-10.
- Toolchain: Borland Turbo Pascal 5.5 (external, not redistributed); DOSBox-X 2026.10.01, executable SHA-256 `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`; Node.js v22.19.0.
- Clean suite: **153/153** direct Pascal sessions (56 standard, 97 standalone), **78/78** configured direct Pascal/production comparisons, E2-10 fixtures F01–F10 passed (238 checkpoints, 824,821 state fields, 169 logical-cycle snapshots); exit code 0 and zero unexplained comparable mismatches.
- Local tested source commit: `29522c73662c1e8b029fff23ba29d5fc01297769`, tree `bdc5cb65db59ff527479b5c31391972afa85857e`. Published mirror: `662dbd57fca3149d01e29d850b35c79c6b4ddb68` in [`kerryhigh/zzt-port`](https://github.com/kerryhigh/zzt-port/commit/662dbd57fca3149d01e29d850b35c79c6b4ddb68).

E2-07 AC003 remains partial: **19/29** source call sites now have direct D/DP caller coverage; 10 lack direct D/DP evidence, including two with integrated-only support. Randomize/time-seed behavior, the register-message call site, and other listed pending callers remain mandatory.
