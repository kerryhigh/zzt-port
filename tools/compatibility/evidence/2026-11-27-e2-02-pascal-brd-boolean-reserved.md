# E2-02 Pascal `.BRD` Boolean and reserved-byte differential

Date: 2026-11-27  
Engine source revision: `2aad843b544c8f71d0ddbbd699f36d9b59bae0b6`  
LibZoo local provenance: `e5d2fe7301828a360e467e20ffd88b6c759db5f5`

## Direct capture and comparison

`BOARDENC.PAS` loads the source-produced `DEMO.ZZT`, sets `Board.Info.IsDark` and `Board.Info.ReenterWhenZapped` to the same raw byte value, and calls the original Pascal `BoardClose` for values `0`, `1`, `2`, and `255`. The probe uses one-byte `Move` setup for these Boolean fields so noncanonical Boolean byte representations reach the original serializer. It also sets `Board.Info.unkPad[70..85]` to the repeating byte pattern `0, 1, 2, 255`.

No reconstructed gameplay or serialization routine is modified. The probe's setup and output capture are the only interventions. `compare-pascal-brd-booleans.js` checks the direct output bytes at both Boolean fields and all 16 reserved positions, imports and re-exports each record through the shared production `.BRD` parser, then compares the complete record. All four 289-byte payloads match with zero differences.

| File | Raw Boolean values (`IsDark`, `ReenterWhenZapped`) | Full file bytes | SHA-256 |
|---|---:|---:|---|
| `BOL0.BRD` | 0 | 291 | `0B276E25F81185D61B62BEF321EA149CF8D08B109ED00DE0CF65C8664EA0C7F1` |
| `BOL1.BRD` | 1 | 291 | `9083E2D7A4DD71C2AB9BE131BE70A08C89CD31804BB06253C7A94B5DD465A77D` |
| `BOL2.BRD` | 2 | 291 | `C93542511E59C390CD31304ED3BA76EC614BFBF4277F05B7EE5970C7890C414B` |
| `BOL3.BRD` | 255 | 291 | `C1FA4E87E5906932A33795BE76D3F0956079C050D0BCB3CB7FFA1CD6D6C57EA5` |

The fixture's Board.Info begins at byte 69 of the 289-byte payload. The tested Boolean bytes are at offsets 70 and 75; the reserved pattern occupies offsets 139 through 154. The complete 289-byte comparison also covers the remaining fields and allocator-sensitive pointer bytes within each same-record parse/export operation; cross-process numeric pointer equality is not required.

## Clean verification

Command (the user-specific external DOSBox-X path is omitted):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain 'build/pascal-compat-run/Z' -DosboxExecutable '<external DOSBox-X 2026.10.01 executable>'
```

Exit code: **0**. Toolchain: Borland Turbo Pascal 5.5, DOSBox-X 2026.10.01, Node `v22.19.0`, and Python `3.13.7`.

- Direct Pascal sessions: **148/148** (54 standard probes; 94 standalone/fixture sessions).
- Configured direct Pascal-to-production comparisons: **69/69** (65 existing, signed `.BRD`, signed `.SAV`, short-string `.BRD`, and Boolean/reserved `.BRD`).
- Integrated replay: **F01–F10, 10/10**, 238 checkpoints, 824,821 state-field comparisons, and 169 logical-cycle snapshots.
- All four Boolean/reserved `.BRD` records matched **289/289** bytes; unexplained comparable mismatches: **0**.

This adds D/DP evidence to the existing frozen E2-02 AC002 record. AC002 remains partial: Boolean variants and reserved/stale tails in the other formats, `DataLen`, board/stat count edges, RLE boundaries, and applicable malformed/truncated inputs remain outstanding. No acceptance IDs or mandatory scope were added.
