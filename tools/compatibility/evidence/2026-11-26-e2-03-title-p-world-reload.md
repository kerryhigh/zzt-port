# E2-03-AC001 — successful title-P world reload

**Result: passed for this route; AC001 remains partial.** The clean-run source revision was `6cb95a04bb9e6edc1c9150a1081617851f5cfee7`. This records additional D/DP evidence for the existing frozen transition case. It does not add an acceptance ID or change the inventory denominator.

## Direct Pascal route

`TITLEIN.PAS` mode `S` loads the same `DEMO.ZZT` with the original `WorldLoad('DEMO', '.ZZT', true)` title-only path, sets the active name to `DEMO`, and sets sentinel live values (`Health=1`, `Ammo=99`). The original `GameTitleLoop` receives the existing queued `P`/`X` tape. Its P branch successfully calls `WorldLoad(World.Info.Name, '.ZZT', false)`, then enters the original board and player loop. The direct capture compares player start, one real paused gameplay cycle, and return to the title loop.

The reconstructed Pascal gameplay routines remain unchanged. Existing staging hooks skip the blocking About modal, seed deterministic RNG/clock/input, capture state, and stop after the named transitions. Turbo Pascal 5.5 compiles the probe against the preserved reference units and DOSBox-X executes it in the clean verification directory.

## Production route

The WASM harness loads the exact Pascal `DEMO.ZZT` bytes as a title-only world, sets the same name and sentinel fields, and supplies an in-memory `DEMO.ZZT` file through the test I/O adapter. It then calls the production `zoo_world_play`, which exercises the real shared `zoo_world_reload`, full `zoo_world_load`, `zoo_board_change`, `zoo_board_enter`, and game tick paths. It captures the same P/C/T checkpoints. The I/O adapter only supplies the fixture bytes; it does not replace world loading or game logic.

## Result

| Checkpoint | Fields compared | Differences |
|---|---:|---:|
| P — successful player start after title-P reload | 3,467 | 0 |
| C/1 — one real player gameplay cycle | 3,467 | 0 |
| T — return to title | 3,426 | 0 |
| **Total** | **10,360** | **0** |

The first-divergence comparator reports checkpoint, field, and neighboring values on failure. The full clean runner exited 0 on 2026-11-26; F01–F10 also passed (238 checkpoints, 824,821 state fields, 169 logical-cycle snapshots). With this added route, the observed inventory is 139 direct Pascal sessions (54 standard, 85 standalone) and 61 configured Pascal/production differential invocations. The verifier's printed direct-session counter was corrected from 84 to 85 standalone sessions after this run because it omitted the newly added title-P execution from its total; the extra Pascal execution and comparison both completed successfully in the clean run.

## Reproducible artifacts

- Probe: `tools/pascal-compat/TITLEIN.PAS`, mode `S`.
- Comparator: `tools/pascal-compat/compare-title-lifecycle.js --saved-world-play`.
- Production harness: `tools/runtime-compat/oop_movement.c::run_integrated_title_saved_world_play_checks`.
- Clean-run Pascal output: `build/pascal-compat-verification/TITLP/INTEGPR.OUT`, 65,591 bytes, SHA-256 `D060FA833D8FFC69AAE4880ADBD49B5A34A7D555E12C9021A63296987894E5ED` (transient; regenerated, not committed).
- Shared world fixture: `build/pascal-compat-verification/TITLP/DEMO.ZZT`, 26,465 bytes, SHA-256 `B54610B5A52B125FA3083F38F6BEEC1CF2178655575FC0D60E999E2659253831` (transient; regenerated, not committed).
- DOSBox-X: 2026.10.01, executable SHA-256 `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`; external toolchain, not redistributed.

This closes the successful counterpart to the previously verified missing-file title-P reload. AC001 remains partial because other save/return outcomes and the wider successful/refused transition matrix remain mandatory.
