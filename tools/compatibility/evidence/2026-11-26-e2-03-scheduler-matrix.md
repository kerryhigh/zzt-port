# E2-03-AC002 — deterministic scheduler matrix

**Result: passed.** This closes the scheduler-matrix acceptance case only. E2-03 remains open because its lifecycle-transition and state-coverage cases are still outstanding.

## Direct Pascal and production comparison

`GAMESCHD.PAS` runs the original `GamePlayLoop` for five cycles in each row. The verification staging copy supplies a deterministic `GetTime` tape to the original `SoundHasTimeElapsed`; it does not replace game-loop or timing semantics. `compare-game-schedule.js` compares the resulting 13 fields per row against the production `zoo_tick` path in `tick_exit.c`.

| Tick speed | Start phase (hsecs) | Stall after poll | Extra PIT ticks | Five cycle phases (hsecs) | Result |
|---:|---:|---:|---:|---|---|
| 0 | 0 | — | 0 | 0, 5, 10, 16, 21 | matched |
| 0 | 5980 | — | 0 | 5980, 5985, 5990, 5996, 1 | matched |
| 1 | 0 | — | 0 | 5, 10, 16, 21, 27 | matched |
| 1 | 5980 | — | 0 | 5985, 5990, 5996, 1, 7 | matched |
| 6 | 0 | — | 0 | 16, 32, 49, 65, 82 | matched |
| 6 | 5980 | — | 0 | 5996, 12, 29, 45, 62 | matched |
| 9 | 0 | — | 0 | 21, 43, 65, 87, 109 | matched |
| 9 | 5980 | — | 0 | 1, 23, 45, 67, 89 | matched |
| 6 | 0 | 2 | 4 | 32, 49, 65, 82, 98 | matched |

All nine rows completed five cycles. The 117 row fields matched with zero differences. The speed-6 rollover output is taken from the compiled Pascal run: the four polls before the first cycle cross the minute boundary, so the observed phase sequence begins `5996, 12, 29, 45, 62`.

The required pause/resume-across-pending-tick case reuses the already completed integrated F07 fixture; no new mandatory fixture was added. Its original Pascal `GamePlayLoop` and production shared `zoo_tick` matched all 13 checkpoints / 44,187 state fields, including delayed pause input, movement while paused, and resume. See [`2026-11-25-e2-10-pause-positive-scheduler.md`](2026-11-25-e2-10-pause-positive-scheduler.md).

## Evidence and controlled inputs

- **D:** compiled Turbo Pascal 5.5 `GAMESCHD.PAS`; the original `GamePlayLoop`, `SoundHasTimeElapsed`, and gameplay state transitions execute. The staged hook controls `GetTime`, stops after five cycles, and captures the direct Pascal values. `VideoWriteText` is a no-op capture boundary, sound output is disabled, and the input tape is empty.
- **DP:** `compare-game-schedule.js` compares all nine 13-field rows with the production `zoo_tick` schedule harness. At speed zero, production returns `RETURN_IMMEDIATE`; positive speeds use `RETURN_NEXT_CYCLE`, matching their original shared-engine contract.
- The clean full Pascal/production runner exited 0: 126/126 direct Pascal sessions, 54/54 configured differentials, all ten integrated fixtures, and the first-divergence diagnostic negative control passed. The standalone runtime verifier also exited 0.
- Direct oracle artifact `GAMESCHD.OUT`: 822 bytes, SHA-256 `bf363aa6f0aed4aec33a474f18ce2f1df5013617b8e840745ced024ed00eac88`.
- Toolchain: Borland Turbo Pascal 5.5; DOSBox-X 2026.10.01, SHA-256 `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`; Node.js 22.19.0; Android NDK 27.2.12479018 clang 18.0.3; Python 3.13.7. The compiler and DOSBox-X are external and are not redistributed.

Reproduce with `tools/pascal-compat/verify.ps1` using the external Turbo Pascal and DOSBox-X paths documented in the repository verification instructions. Oracle instrumentation is generated in the ignored clean staging directory; reconstructed Pascal gameplay semantics are not modified.

## Remaining E2-03 work

Initial load, title-to-play, board transitions, passages, death, restore, pause/resume, save/return, quit/abort, malformed-load failure, and refused or blocked transitions still need their finite lifecycle coverage and direct comparisons. Passing this schedule matrix does not close E2-03.
