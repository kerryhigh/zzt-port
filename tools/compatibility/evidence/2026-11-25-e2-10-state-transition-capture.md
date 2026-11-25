# E2-10 integrated state-transition capture — 2026-11-25

**Classification:** DP (direct compiled-Pascal / production differential), for the existing `INTEGPRB` prototype only.  
**Result:** matched. The prototype remains infrastructure evidence and is not one of the ten required fixtures.

## Checkpoint capture

The staged Turbo Pascal `GAME.PAS` and the production test harness now capture a named state snapshot when the board, game role, or paused state changes. The existing fixture begins paused and moves east, so it exercises one observed pause-to-play transition. The Pascal hook records this transition immediately after the original unpause branch has updated the game state; production records it after the matching `zoo_tick` returns. A normalized scalar maps the Pascal `E_PLAYER` / `E_MONITOR` marker and production `GS_PLAY` / `GS_TITLE` state to player / monitor / other values without comparing unrelated enum numbers.

The comparator now validates ordered identities and sequential IDs for loaded state, loop start, transitions, six cycles, and final state. This run produced `L0`, `S0`, `T1`, `C1` through `C6`, and `F0`. Each snapshot retains the existing world/stat/scheduler/input/tile/sentinel, RNG, pointer-alias graph, unique script bytes, sound-queue, and cumulative cell-write fields.

| Checkpoint | Fields | Result |
|---|---:|---|
| Loaded `L0`, loop start `S0`, final `F0` | 3,473 each | matched |
| Pause-to-play `T1`, cycles `C1`–`C6` | 3,475 each | matched |

All ten checkpoints matched across **34,744 state-field comparisons**. Two logical sound queue events matched across 20 fields. Nine ordered playfield cell writes matched across 36 fields with no capture overflow.

The first comparison exposed a probe-boundary mismatch: Pascal's transition snapshot was taken at the end of the repeat iteration, after its zero-speed cycle advanced, while production returned from `zoo_tick` at the transition boundary. The first difference was `current tick` (57 vs. 56) and `current stat tick` (0 vs. 3). The staged Pascal hook now captures immediately after the existing unpause-state assignments, before the later cycle boundary. The subsequent full differential matched. This corrected checkpoint alignment; neither the Pascal reference gameplay nor production gameplay semantics were changed.

## Outputs and verification

Generated outputs are build artifacts and are not committed:

| Output | Size | SHA-256 |
|---|---:|---|
| `INTEGPR.OUT` | 73,589 bytes | `1cf35bd4fa803f17799c1589461cf63aa238a5c65dfa6fc33b5af734284c3b29` |
| `INTEGSND.OUT` | 53 bytes | `238fc22ca3707890778e1303541f118a99110fdd9f1ba84f1d3ee7fedb07b0be` |
| `INTEGCEL.OUT` | 103 bytes | `6f153a1c437a476912a973144f6ee3e8e483b6478dbc16b9039e58bad0d246fa` |

- `tools/pascal-compat/verify.ps1` from a clean staging directory — exit 0; 84/84 direct Turbo Pascal 5.5 sessions and 43/43 configured Pascal/production comparisons passed; zero unexplained mismatches in those configured cases.
- `tools/runtime-compat/verify.ps1` — exit 0.
- The integrated comparison ran inside the Pascal suite: ten checkpoints / 34,744 state fields, 20 sound-event fields, and 36 cell-write fields matched.
- Toolchain: Borland Turbo Pascal 5.5 (`TPC.EXE`, `TURBO.TPL`, excluded from the repository); DOSBox-X 2026.10.01, executable SHA-256 `072c1944635ccc545a816e128625e3405feab559021e4ef60b2d512f97f754c0` (external, not redistributed); Node.js 22.19.0; Python 3.13.7.
- Public verification records omit the user-specific absolute path to the external DOSBox-X executable. Supply the matching executable with `-DosboxExecutable` when running the clean Pascal verifier.

The current replay still does not capture generated save/file bytes or return/error results and does not complete the ten-fixture corpus. E2-10 remains open at **0/10 fixtures**; this replay is one three-stat prototype only.
