# E2-02 signed world-header boundary differential

Date: 2026-11-27  
Engine source revision: `eb33726d49b71af239b9024b661bec0bf536bf7d`  
LibZoo local provenance: `e5d2fe7301828a360e467e20ffd88b6c759db5f5`

## Probe and comparison

The direct compiled-Pascal `WORLDSAV.PAS` probe loads the source-produced `DEMO.ZZT`, leaves its current board valid, and calls the original `WorldSave` operation for five `.SAV` files. For each file, it assigns the same signed boundary value to nine serialized world fields: `Ammo`, `Gems`, `Health`, `Torches`, `TorchTicks`, `EnergizerTicks`, `Score`, `BoardTimeSec`, and `BoardTimeHsec`. The values are `-32768`, `-1`, `0`, `1`, and `32767`. The existing quiet video callback suppresses probe display output; it does not replace loading, saving, or gameplay semantics. No reconstructed Pascal gameplay or serializer routine was modified.

`compare-pascal-signed-world.js` reads every Pascal save through the shared production world reader and verifies all nine fields against the expected signed value. It then loads the same DEMO world, opens the valid current board, assigns the same values, invokes production `zoo_world_save`, and compares the complete output file byte-for-byte. Each Pascal file is 26,465 bytes; all five production files matched every byte.

| Pascal value | Staging file | Bytes matched | SHA-256 | Result |
|---:|---|---:|---|---|
| -32768 | `WRS0.SAV` | 26,465/26,465 | `CF3A68497C04277A9499D83229F0E340FF62D6494CF3CEF5B3A4C580003C6862` | D/DP pass |
| -1 | `WRS1.SAV` | 26,465/26,465 | `CADD98A9BA641685ECA91EDE70DD156F4EBCDC2D9C0A4A3E730E2E33A04DDEBD` | D/DP pass |
| 0 | `WRS2.SAV` | 26,465/26,465 | `08EAC392DED45CC7883BC137092CA0360D704A8925D55CFA6952D673836164B2` | D/DP pass |
| 1 | `WRS3.SAV` | 26,465/26,465 | `232D7BCB5F3E7E4EB7C5B945E8FD5121A96027BEC3B20EDFBBF3296FDE23540D` | D/DP pass |
| 32767 | `WRS4.SAV` | 26,465/26,465 | `7362C7AF55C2439C2B6B87A480E96695EB785331BC4D8C471827A6E85314F965` | D/DP pass |

These `.SAV` files are generated under the ignored clean staging directory and are not committed. The tracked probe and comparator regenerate them.

## Clean verification

Command (the user-specific external DOSBox-X path is omitted):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain 'build/pascal-compat-run/Z' -DosboxExecutable '<external DOSBox-X 2026.10.01 executable>'
```

Exit code: **0**. Toolchain: Borland Turbo Pascal 5.5; DOSBox-X 2026.10.01 (executable SHA-256 `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`); Node `v22.19.0`; Python `3.13.7`.

- Direct Pascal sessions: **148/148** (54 standard probes, 94 standalone/fixture sessions).
- Configured direct Pascal/production comparison invocations: **67/67** (65 existing comparisons, five-value signed `.BRD` comparator, and this five-value signed `.SAV` comparator).
- Integrated replay: **F01–F10, 10/10**, 238 checkpoints, 824,821 state-field comparisons, and 169 logical-cycle snapshots.
- F10 post-failed-`WorldLoad` state: **3,385/3,385** comparable fields; private `World.BoardLen[0]` excluded by the existing implementation-specific state rule.
- Unexplained comparable mismatches in this run: **0**.

This adds D/DP evidence to the existing frozen E2-02 AC002 case; it adds no acceptance case and does not close AC002. The remaining per-format short-string and raw-length cases, embedded NUL/CP437 bytes, Boolean variants, reserved and stale tails, `DataLen`, count limits, RLE boundaries, and applicable malformed/truncated input matrix remain outstanding.
