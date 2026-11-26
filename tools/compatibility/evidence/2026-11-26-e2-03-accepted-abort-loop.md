# E2-03-AC001 — accepted player abort across the gameplay-loop boundary

**Result: D/DP passed for the accepted player-abort route.** E2-03-AC001 remains partial; this adds evidence to the frozen case and does not add an acceptance ID or integrated-corpus fixture.

## Probe and comparison

`tools/pascal-compat/ABORTLP.PAS` loads the checked-in DEMO world and enters the original compiled-Pascal `GamePlayLoop(true)`. It sets the player loop paused, queues Escape, scripts `Y` at the existing `GamePromptEndPlay` input boundary, and supplies a deterministic zero-time schedule. The original `GamePlayLoop`, prompt decision routine, and gameplay state transitions run unchanged. The capture is written only after the original loop returns with `GamePlayExitRequested` set and the Escape/key buffers consumed.

The production side in `tools/runtime-compat/oop_movement.c::run_integrated_end_play_loop_check` loads the identical `DEMO.ZZT` bytes and enters shared play state. At the matching resolved-decision boundary it calls `zoo_game_resolve_end_play(&state, true, true)`, then runs the shared `zoo_tick` exit transition and captures the corresponding player, world, scheduler, and sound state. This compares the accepted exit across the game-loop boundary; it does not claim equivalence for frontend prompt drawing or physical key routing.

The sound setup mirrors the direct Pascal probe's explicit pre-loop initialization. Pascal stores `SoundBufferPos` as a one-based index; LibZoo stores the same empty-buffer position as a zero-based offset, so the comparator reports the production position plus one. This is an index representation mapping, not an omitted or normalized behavior difference.

## Result

Direct Pascal output `build/pascal-compat-verification/ABORTLP.OUT` contains 22 fields and is 51 bytes, SHA-256 `FAEF2D8E8EE74B138178B061437AF70DACC8F792ECA498D155FB0B731C680F65`. Its captured row is:

```text
1 1 0 0 0 4 0 0 1 1 100 0 0 3 30 12 4 31 0 1 0 -1
```

The shared production capture matched all **22/22** fields: exit flag, paused state, key and input-buffer state, game/stat ticks, scheduler duration/counter, current/return board, health/ammo/save flag, stat count, player coordinates and tile, sound blocking, buffer position/length, and sound priority. The exact DEMO fixture loaded by the production replay is 26,465 bytes, SHA-256 `B54610B5A52B125FA3083F38F6BEEC1CF2178655575FC0D60E999E2659253831`.

## Verification and scope

- Evidence class: **D+DP**. Turbo Pascal 5.5 compiled and ran the original `GamePlayLoop`; the direct result was compared field-for-field with the shared LibZoo loop-exit boundary.
- The clean full runner exited 0 on 2026-11-26. It passed 142 direct Turbo Pascal compile/run sessions (54 standard probes and 88 standalone sessions), 62/62 configured differential invocations, all ten frozen integrated fixtures, and the runtime suite. The accepted-abort loop is an additional comparison inside the existing player-abort differential invocation.
- Staging controls queue Escape and Yes at existing input boundaries and provide a deterministic clock; no Pascal gameplay routine was modified. The production setup explicitly matches the Pascal pre-loop sound state.
- This covers the accepted Yes route only. Refused No/Escape decisions are covered separately, but their full post-decision loop/title-return states, remaining save/return routes, and the broader successful/refused board/world transition matrix remain open. The frozen E2-10 corpus remains 10/10; this probe receives no E2-10 fixture credit.

Toolchain: Borland Turbo Pascal 5.5 (`TPC.EXE`/`TURBO.TPL`, external and not redistributed), DOSBox-X 2026.10.01, and Node.js v22.19.0. DOSBox-X executable SHA-256: `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`.
