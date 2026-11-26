# E2-10-F10 — malformed-world recovery and high-score insertion

**Result:** passed in the clean Pascal/production runner on 2026-11-26. This closes fixture F10 only; Engine 2.0 and E2-11 remain open.

## Replay and comparison

The direct Turbo Pascal 5.5 probe loads a deterministic malformed 512-byte `.ZZT` (`F10BAD.ZZT`). Original Pascal `WorldLoad` returns false (`F10BAD.OUT` contains `0`). It then follows the startup recovery setup with `WorldCreate`, loads the absent `F10ROOM.HI` table through `HighScoresLoad`, and enters the original `GamePlayLoop` for two logical cycles. The death path runs `HighScoresAdd`; a normalized input tape enters `KERRY` and Enter, and the original Pascal code writes the `.HI` file. The probe also saves, reloads, and saves the world through the original `.SAV` routines.

The shared-engine replay presents the same malformed bytes to `zoo_world_load`, which returns `ZOO_ERROR_WRONGVER` (`-3`), applies the corresponding `zoo_world_create` recovery, runs the production `zoo_tick` loop for two cycles, invokes the shared high-score load/reset, insertion, and save APIs, and round-trips the `.SAV`. The high-score table is compared byte-for-byte against the Pascal output.

| Check | Pascal / production result |
| --- | --- |
| Malformed startup world | Pascal `WorldLoad=false`; production `ZOO_ERROR_WRONGVER (-3)` |
| Gameplay snapshots | 7 checkpoints: `L0`, `S0`, `C1`, `C2`, `T2`, `T3`, `F0`; 23,861 state fields matched; 0 differences |
| Logical sounds | 0 events; 0 differences |
| Ordered gameplay cell writes | 29 writes / 116 fields; 0 differences |
| High-score insertion | `KERRY`, score 1234 at rank 1; all 1,590 `.HI` bytes matched; 0 differences |
| Generated `.SAV` | 866 bytes; 610 stable bytes matched; 256 undefined/pointer positions classified; 0 stable-byte differences |
| Loaded-file `.SAV` round trip | 866 bytes; 858 preserved bytes matched; 8 process-pointer positions classified; 0 stable-byte differences |

The integrated runner reported:

```text
Integrated gameplay replay (malformed-load-high-score-insertion): 7 checkpoints (3399, 3399, 3399, 3416, 3416, 3416, 3416 fields) matched (23861 comparisons); 0 logical sound-queue events matched (0 event fields); 29 playfield cell writes matched (116 event fields); malformed startup .ZZT recovery returned Pascal=false/production=WRONGVER and 1590-byte .HI insertion matched; generated fresh-world SAV matched 610 stable bytes with 256 undefined/pointer positions classified (none); loaded-file round-trip SAV matched 858 preserved bytes with 8 process-pointer positions classified; Pascal WorldLoad=true matched production zoo_world_load=0; first-divergence reports identify checkpoint/event, field, tile, cell, serialized byte, and surrounding values
```

## Controlled inputs and interventions

- `F10BAD.ZZT` is exactly 512 bytes; its first word is `0xFFFE`. SHA-256: `378621df4e6f111e6ce670297eeb7a21a6613732b88b22d79619124cd7894c2f`.
- The `.HI` file is removed before the probe. The Pascal table payload is first seeded with known short-string tails, then `HighScoresLoad` exercises the missing-file reset. Production seeds the same payload bytes before the shared reset API. The entered name and score are determined by the input tape and fixture state.
- `RandSeed` is set to zero and the original transition-table generation runs before `WorldCreate`. The Pascal probe sets `World.Info.unk1` to zero before the malformed load; the failed load and fallback create preserve that controlled value.
- The probe uses the same Turbo Pascal memory directive as reconstructed `ZZT.PAS`: `{$M 49152,163840,655360}`. This supplies the stack needed by the original high-score text-window path; omitting it caused a harness-only runtime error 202. The reference gameplay source was not changed.
- The staged clock hook stops the original `GamePlayLoop` after two completed cycles. `InputProbeSetReadKeys` supplies `KERRY` and Enter to the existing name prompt. Gameplay, `HighScoresLoad`, `HighScoresAdd`, `HighScoresSave`, `WorldSave`, and `WorldLoad` remain reconstructed Pascal routines.
- The staged cell-write observer is disabled immediately before the original high-score text window runs, so presentation-only modal writes do not enter the gameplay-cell comparison. No engine state or gameplay routine is substituted.
- The production replay runs the actual `zoo_tick` loop and shared high-score module. Its name input is supplied at the shared engine API boundary; the platform name-entry UI remains outside this platform-independent engine fixture.
- Turbo Pascal compile and execution use separate fresh DOSBox-X 2026.10.01 sessions. The DOSBox-X executable is external and not redistributed; SHA-256 `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`.

## Clean-run record

- Command: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain build/pascal-compat-run/Z -DosboxExecutable <external DOSBox-X 2026.10.01 executable>`; exit code 0.
- Runtime command: `tools/runtime-compat/verify.ps1`; exit code 0.
- Direct Pascal sessions: 126/126 (54 standard probes and 72 standalone sessions).
- Configured Pascal-to-production comparisons: 54/54; no unexplained mismatch.
- Required integrated fixtures: F01–F10 passed. The F05/F08 identical-value redraw counts remain classified separately by the existing cadence rule.
- F10 output hashes:

| Output | Bytes | SHA-256 |
| --- | ---: | --- |
| `F10BAD.OUT` | 3 | `13bf7b3039c63bf5a50491fa3cfd8eb4e699d1ba1436315aef9cbe5711530354` |
| `F10ROOM.HI` | 1,590 | `cd54167c2bc7722f69368d1bbc2ea74cf9589da1aeb6a422fb9972ee43aa4078` |
| `INTEGPR.OUT` | 50,260 | `36feaa801103188b17e949fd71e94fb7ed9c5642fe855c7a3af8feb23962eb14` |
| `INTEGSND.OUT` | 5 | `043b5ae7334040ef63298adca08912d5287ad37e66f9425316adc50d2c1ab652` |
| `INTEGCEL.OUT` | 364 | `c6cd5b5e0a8e85ae9ff77f552ee2c03f828ee1ef6ea967e6e0aba5335df44eac` |
| `INTEGSAV.SAV` | 866 | `0da4af3f9e7a64b0419757187688a310a637f6a9ea52e281ba4e732e004f6754` |
| `INTEGLOD.OUT` | 3 | `f1b2f662800122bed0ff255693df89c4487fbdcf453d3524a42d4ec20c3d9c04` |
| `INTEG2SV.SAV` | 866 | `0da4af3f9e7a64b0419757187688a310a637f6a9ea52e281ba4e732e004f6754` |

`F10IN.PAS` is the direct oracle probe. The clean-run implementation is recorded at local code commit `315573177c1bd4f0e7c20f5525d03b16f23dd5ab`; generated DOSBox and build outputs are not committed.
