# E2-10 integrated sound-queue capture — 2026-11-24

**Classification:** DP (direct compiled-Pascal / production differential) for the existing integrated prototype only.  
**Result:** matched. The prototype still does not count as one of the ten required fixtures.
**Engine harness revision:** `a7642fdd3f222747e93f17771c195d3926f89ff1`.

## Capture and comparison

The direct Turbo Pascal 5.5 integrated probe ran the reconstructed `GamePlayLoop` for six deterministic zero-speed cycles. Its clean staging copy records `SoundQueue` requests after the unmodified source routine returns, including request priority and bytes plus resulting queue priority, 1-based position, length, playing state, and queued bytes. Events are written to a separate `INTEGSND.OUT` file after gameplay. The uninstrumented reference file is unchanged and still matches `reference_source_hashes.sha256`: `SOUNDS.PAS` SHA-256 `9a24453f13ea86cf832719f204bef135b41a0fd6fed4c1d80657551516406378`.

The deterministic probe explicitly disables sound output and initializes the sound queue fields before the replay; no sound timer interrupt is installed. The event hook does not run gameplay or sound-queue logic of its own. Its fixed 64-event capture storage is allocated only after the probe's script fixture has been created, and overflow is a hard comparison failure.

The existing nine checkpoints now include sound queue fields and live queue bytes. Pascal and production matched these checkpoint sizes and all **31,242** state fields:

| Checkpoint | Compared fields | Result |
|---|---:|---|
| Loaded initial state | 3,470 | matched |
| Game-loop start | 3,470 | matched |
| Cycles 1–6 | 3,472 each | matched |
| Final state | 3,470 | matched |

The replay emitted **2** logical sound-queue calls, each requesting priority 2 and bytes `[21, 1]`; both post-call queues matched. The comparator checked **20** event scalar/byte fields and reported no mismatch or capture overflow. `INTEGPR.OUT` is 66,175 bytes (SHA-256 `70f50cd664b5b439458b2aef243f9b8376f9f8808cd3ed8afffef7f33881c35e`); `INTEGSND.OUT` is 53 bytes (SHA-256 `238fc22ca3707890778e1303541f118a99110fdd9f1ba84f1d3ee7fedb07b0be`). The outputs are generated build artifacts and are not committed.

## Verification commands and limits

- `powershell -NoProfile -ExecutionPolicy Bypass -File tools/runtime-compat/verify.ps1` — exit 0 after the harness update.
- `node tools/pascal-compat/compare-integrated-gameplay.js build/pascal-compat-verification/INTEGPR.OUT build/runtime-compatibility/integrated_gameplay.wasm build/pascal-compat-verification/INTEGSND.OUT` — exit 0.
- A full `tools/pascal-compat/verify.ps1` attempt compiled and ran `INTEGPRB`, then stopped when DOSBox-X did not exit from the subsequent `BRDXFER` session within the runner's 30-second limit. Re-running that session separately with a 60-second limit also timed out. Therefore this attempt is **not** recorded as a passing clean full Pascal suite; the previously recorded full-suite result remains the last complete suite result.

This evidence covers sound queue state/events only for the existing three-stat prototype. It does not cover logical cell writes, distinct board/state-transition checkpoints, serialized output bytes, the ten-fixture integrated corpus, or all sound producers and timer boundaries required by E2-08/E2-10.
