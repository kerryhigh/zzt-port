# E2-10 same-file save/load round-trip

**Date:** 2026-11-25  
**Evidence:** DP (compiled reconstructed Pascal and production LibZoo)  
**Status:** Partial evidence for E2-10 AC001/AC004; neither case nor E2-10 is closed.

## Replay and controlled setup

The `INTEGPRB` replay continues to execute the reconstructed Pascal `GamePlayLoop` and the production `zoo_tick` path through six zero-speed cycles, a pause-to-play transition, two boulder pushes, and two interacting `#GIVE` / `#IDLE` objects. Its checkpoints are `L0`, `S0`, `T1`, `C1`–`C6`, `T2` (first save boundary), `T3` (reload boundary), and `F0` (final state). The clean comparison matched 41,690 state fields, two logical sound requests / 20 event fields, and nine ordered cell writes / 36 event fields.

The oracle gameplay source remains untouched. The test explicitly sets the otherwise-uninitialized `World.Info.unk1` word to zero to match the zeroed production fixture allocation; this is test setup only. The verifier compiles `INTEGPRB` in one DOSBox-X invocation and executes the compiled program in a fresh DOSBox-X session. A same-session compile/run terminated after cycle capture and before producing the cell sidecar; executing the same compiled probe in a new session completed. This staging separation controls inherited DOS allocation history without substituting or altering gameplay routines.

## First save

The compiled Pascal run produced `INTEGSAV.SAV` (961 bytes; SHA-256 `378a6aaae4ef3f8f040f720c006a8896c6a52d4b672b599d0367fb3ace704700`). The production `zoo_world_save` result was `0`. The fresh-world policy matched all 565 stable positions and explicitly classified 396 positions from undefined Pascal string tails/padding or process-specific pointers; 327 classified byte values differed and 69 happened to match. The mask does not apply to the subsequent load.

## Same-file load and second save

Both implementations loaded the exact Pascal-generated `INTEGSAV.SAV` input. The oracle sidecar `INTEGLOD.OUT` records `WorldLoad=true` (3 bytes including its line ending); production `zoo_world_load` returned `0`. The run then generated `INTEG2SV.SAV` (961 bytes; SHA-256 `8b4d70ab16e3c730f592197a508b409d0bf0e6e244491bd979b738e75466b0e3`). The loaded-file comparator masks only the 12 stat data-pointer positions that may differ by process allocation. All other 949 bytes—including the original raw string tails, unknown record bytes, coordinates, reserved bytes, and defined state—matched exactly. There were zero stable-byte divergences.

## Clean full-run evidence

The full `tools/pascal-compat/verify.ps1` run completed with exit code 0 after compiling and executing the integrated probe in separate fresh DOSBox-X sessions. It passed 84/84 direct Turbo Pascal 5.5 sessions, 43/43 configured Pascal-to-production comparisons, and the runtime compatibility suite. Runtime verification rebuilt the Wasm harness and passed. The latest generated outputs were:

| Output | Size | SHA-256 |
|---|---:|---|
| `INTEGPR.OUT` | 88,305 | `e302935a10fbcbabdc7d39ba44c99e2bcc9a6e05bdf7951b26c6833059f10efe` |
| `INTEGSND.OUT` | 53 | `238fc22ca3707890778e1303541f118a99110fdd9f1ba84f1d3ee7fedb07b0be` |
| `INTEGCEL.OUT` | 103 | `6f153a1c437a476912a973144f6ee3e8e483b6478dbc16b9039e58bad0d246fa` |
| `INTEGSAV.SAV` | 961 | `378a6aaae4ef3f8f040f720c006a8896c6a52d4b672b599d0367fb3ace704700` |
| `INTEGLOD.OUT` | 3 | `f1b2f662800122bed0ff255693df89c4487fbdcf453d3524a42d4ec20c3d9c04` |
| `INTEG2SV.SAV` | 961 | `8b4d70ab16e3c730f592197a508b409d0bf0e6e244491bd979b738e75466b0e3` |

## Limits

This still is one synthetic replay prototype, not a completed member of the required ten-fixture corpus; the corpus remains 0/10. It demonstrates one save, load, and re-save path from one generated world. It does not cover all `.SAV` error paths, save/restore behavior over varied worlds, board changes, malformed files, or the other required scenario coverage. Pascal `WorldSave` is a procedure without a return value, so the failure-path outcome remains open. No fixture requirement, inventory denominator, or blocker status changed.
