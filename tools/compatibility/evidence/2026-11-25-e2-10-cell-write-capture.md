# E2-10 integrated playfield cell-write capture — 2026-11-25

**Classification:** DP (direct compiled-Pascal / production differential) for the existing integrated prototype only.  
**Result:** matched. The prototype remains infrastructure evidence and is not one of the ten required fixtures.

## Capture and comparison

The staged Turbo Pascal 5.5 `INTEGPRB` probe now records each `VideoWriteText` character whose destination lies in the 60×25 playable area. Each event stores zero-based screen coordinates, the original color attribute byte, and the CP437 character byte. Writes to the sidebar and other screen cells are excluded from this engine-only trace. Capture is disabled during the loaded/start setup and final cleanup; the checkpoints store the cumulative event count, allowing the ordered events to be partitioned at each cycle boundary.

The staged Pascal callback only observes `VideoWriteText`; it does not alter reference gameplay or rendering semantics. It writes a separate numeric `INTEGCEL.OUT` sidecar with a fixed 8,192-event capacity. Overflow or malformed events fail comparison. Production records the corresponding `zoo_video_driver.func_write` calls through the integrated harness, with the same bounds, event fields, and capacity.

The first differential run exposed a fixture setup error: the production test harness zeroed `zoo_state` directly and bypassed `zoo_state_init`, leaving `player_character` at zero. The normal production initializer and Pascal `ElementDefs[E_PLAYER].Character` both start at byte 2. The fixture now explicitly seeds byte 2 before gameplay. No reference gameplay code or production gameplay behavior changed to resolve the mismatch.

The regenerated direct Pascal output matched the production harness:

| Checkpoint | Compared state fields | Result |
|---|---:|---|
| Loaded initial state | 3,472 | matched |
| Game-loop start | 3,472 | matched |
| Cycles 1–6 | 3,474 each | matched |
| Final state | 3,472 | matched |

Across nine checkpoints, **31,260** state fields matched. The two sound queue events (20 compared event fields) still match. The ordered playfield trace contains **9 cell writes** and **36 event fields**, all matched, with no capture overflow. `INTEGPR.OUT` is 66,211 bytes (SHA-256 `38dd3d676a26620c0068c62a8be9a43250aa94f6977da3a20943bd951fb8683a`); `INTEGSND.OUT` is 53 bytes (SHA-256 `238fc22ca3707890778e1303541f118a99110fdd9f1ba84f1d3ee7fedb07b0be`); `INTEGCEL.OUT` is 103 bytes (SHA-256 `6f153a1c437a476912a973144f6ee3e8e483b6478dbc16b9039e58bad0d246fa`). These outputs are generated build artifacts and are not committed.

## Verification commands and limits

- `powershell -NoProfile -ExecutionPolicy Bypass -File tools/runtime-compat/verify.ps1` — exit 0 after adding the normalized player-glyph fixture setup and trace capture.
- `node tools/pascal-compat/compare-integrated-gameplay.js build/pascal-compat-verification/INTEGPR.OUT build/runtime-compatibility/integrated_gameplay.wasm build/pascal-compat-verification/INTEGSND.OUT build/pascal-compat-verification/INTEGCEL.OUT` — exit 0; 31,260 snapshot fields, 20 sound event fields, and 36 cell-write fields matched.
- The encompassing `tools/pascal-compat/verify.ps1` attempt compiled and ran the integrated probe and subsequent editor probes, then stopped when DOSBox-X did not exit from `BRDXFER` within the runner's 30-second limit. This is not a passing full Pascal-suite result. The previous last completed suite remains 84 direct sessions and 43/43 configured differentials on `c7c49c3`.

This result verifies the existing three-stat replay only. The ten-fixture corpus, distinct state-transition checkpoints, generated file bytes, broader cell-write scenarios, and all sound producers/timer boundaries remain outstanding under E2-08 and E2-10.
