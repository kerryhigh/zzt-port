# E2-03: direct title-W world selection and play replay

**Acceptance case:** `E2-03-AC001` (existing frozen case; no new case added)  
**Evidence:** D + DP  
**Result:** passed for this route; AC001 and E2-03 remain partial.

## Route and result

The Turbo Pascal 5.5 probe executes the original `GameTitleLoop`: it begins with the W input path, selects the available `DEMO.ZZT`, enters play through P, runs one real paused gameplay cycle, then returns to the title lifecycle capture. The direct Pascal transcript and the shared production engine replay matched at all 3 checkpoints, across 10,360 state fields. The comparison uses `compare-title-lifecycle.js --world-select-play`; a mismatch reports the first differing checkpoint and field, both values, and neighboring field context.

The replay is additional lifecycle evidence for AC001. It does not add a fixture to the frozen E2-10 corpus and does not close E2-03.

## Differential and implementation finding

The replay exposed a scheduler initialization ordering defect in `zoo_world_play`. Production had initialized `GS_PLAY` before changing to and entering the selected board. In the Pascal lifecycle, the return board is selected, `BoardChange`/`BoardEnter` execute, and only then does the play state initialize its scheduler from the entered board. The production path now follows that order and calls `zoo_game_start_preserving_input(GS_PLAY)` after board selection and entry.

The direct comparison passed after the production correction. This preserves the shared engine implementation; Android and Windows continue to use the same engine code.

## Oracle output and fixtures

- Pascal output: `build/pascal-compat-verification/TITLWLD/INTEGPR.OUT`, 85,500 bytes, SHA-256 `7446bad95daff2c933ab90bc6b4009ca5b40a3466bb3d04d66d2d848ccb1afd6`.
- DOS world fixture: `build/pascal-compat-verification/TITLWLD/DEMO.ZZT`, 26,465 bytes, SHA-256 `b54610b5a52b125fa3083f38f6beec1cf2178655575fc0d60e999e2659253831`.
- These are transient clean-staging outputs. The documented verification runner recreates them; they are not committed as oracle output files.
- Reference source identity is recorded in [`reference_source_hashes.sha256`](../reference_source_hashes.sha256). Turbo Pascal 5.5 and DOSBox-X are external verification dependencies and are not redistributed.

## Controlled inputs and interventions

`TITLEIN.PAS` is a probe compiled against the reconstructed Pascal units. It prepares the controlled `DEMO.ZZT` state, sets `RandSeed` to zero, disables audible sound and initializes the sound capture schedule, seeds the actual `GameTitleLoop` with `W`, enables the existing snapshot hooks, and uses `TickSpeed = 0` for one paused-cycle observation. The clean runner stages the probe and fixture in a temporary DOS working directory.

`tools/pascal-compat/verify.ps1` makes two route-specific input insertions in the temporary staged `GAME.PAS`: it injects Enter immediately before the original `GameWorldLoad('.ZZT')` selector call, then queues P after a successful load. The original `GameWorldLoad`, `TextWindowSelect`, `WorldLoad`, board entry, `GamePlayLoop`, and `GameTitleLoop` bodies execute. These are deterministic input/capture interventions; they do not replace the Pascal lifecycle routines.

The production replay receives the identical `DEMO.ZZT` bytes through its deterministic memory I/O handle. It exercises the shared world-load/play engine path, but this comparison does not cover host filesystem access, DOS path resolution, directory enumeration, or selector presentation. Those remain outside this route's evidence.

The oracle transcript itself is not edited to match production. Test instrumentation is maintained separately from the reconstructed source and its hashes; this route does not modify reference gameplay semantics.

## Full clean-run context

On 2026-11-26 the documented full Pascal runner exited 0: 147 direct Turbo Pascal sessions (54 standard and 93 standalone), 65/65 configured direct Pascal/production comparisons, and the frozen integrated replay set F01-F10 passed at 238 checkpoints, 824,821 compared state fields, and 169 logical-cycle snapshots. The runtime compatibility verifier passed. Android CP437/input compatibility and Android debug/instrumentation builds passed. The Windows x64 build passed and CTest passed 3/3. No physical Android device smoke was performed in this verification record.

Toolchain recorded by the run: Borland Turbo Pascal 5.5 (external; `TPC.EXE` and `TURBO.TPL` excluded), DOSBox-X 2026.10.01 (external executable SHA-256 `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`), Node v22.19.0, Python 3.13.7, MSBuild 17.14.19 / Visual Studio 2022 v143 x64. Engine source revision: `19f38210ff3fba9e2e930eddbbd8c27406ae2851`.

## Remaining scope

This route verifies only one title-driven world-selection/play/return path under its recorded fixture and controlled inputs. Other successful and refused lifecycle transitions, filesystem-backed world selection, and the remaining AC001 matrix still require the evidence specified by the checklist. E2-03 remains partial; the frozen acceptance inventory remains 54 cases; E2-10 remains the only closed blocker.
