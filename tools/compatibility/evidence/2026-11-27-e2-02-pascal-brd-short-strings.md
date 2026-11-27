# E2-02 Pascal `.BRD` short-string boundary differential

Date: 2026-11-27  
Engine source revision: `8204eb912f30053ab72805c33ce8c6c433927295`  
LibZoo local provenance: `e5d2fe7301828a360e467e20ffd88b6c759db5f5`

## Direct capture and comparison

`BOARDENC.PAS` loads the source-produced `DEMO.ZZT` once, then uses the original Pascal `BoardClose` routine to emit nine `.BRD` records. The first eight set the raw `Board.Name` length byte to `0`, `1`, `49`, `50`, `51`, `127`, `255`, and `50`; the 50-byte case includes `A`, NUL, and CP437 bytes `0x80`, `0x82`, and `0xFF`. The ninth assigns `ZZT-STALE-TAIL-TEST` and then `X`, capturing the tail left by Pascal short-string assignment.

The probe changes only setup fields and output capture; it does not patch reconstructed gameplay or serialization routines. It reuses the existing byte buffer and loaded board while producing these records. An earlier probe version with extra static arrays and a second `WorldLoad` timed out before writing output; the final probe avoids both additions and completed in the clean suite.

`compare-pascal-brd-strings.js` checks the two-byte length word, raw name length, the embedded NUL/CP437 bytes, and the stale tail. It then imports and re-exports each record through the shared production `.BRD` parser and compares all 289 record bytes. All nine cases passed with zero byte differences.

| File | Raw name length | Full file bytes | SHA-256 |
|---|---:|---:|---|
| `STR0.BRD` | 0 | 291 | `D2E16E26ED65B6BCA66950240137D83757A0A36BDE2DD091B5019C4E008F4CBB` |
| `STR1.BRD` | 1 | 291 | `2CDFE5EF9839C526098933975D53551F6AE7315E8562BBF23BF854D125A52B9F` |
| `STR2.BRD` | 49 | 291 | `A909DCF94E6D32E4ACFED201FE1B6F13764995FEB96E2F6C8A70E2734C4F5AA1` |
| `STR3.BRD` | 50 | 291 | `742B709AD321AD45C94D5554E98D88CAA5C5331317A63BBB2DAF19614ED120B1` |
| `STR4.BRD` | 51 | 291 | `A5C61E861E06ED9DF92674C971885DB8F89D3F0E5A638376514DBA4E49BB4130` |
| `STR5.BRD` | 127 | 291 | `7B91FD88BE17D47692FFB1730782ECB0F1CF94CE6F88DEA2D04B0FE119E25220` |
| `STR6.BRD` | 255 | 291 | `FA40581C656C97D6875CDF98A33AD4FAAA0F1F3897B84DEFD8938C1554C6ADEC` |
| `STR7.BRD` | 50 (embedded bytes) | 291 | `C3ED92272B0EA42BEBC2A603BB67F5FAD642F4AB2C17ECC52DA6287A24155A6C` |
| `STR8.BRD` | 1 (stale tail) | 291 | `AEB237A6F02956E6C3682036CECE233881791E20B4C5569F4EE1918FE11D7109` |

## Clean verification

Command (the user-specific external DOSBox-X path is omitted):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain 'build/pascal-compat-run/Z' -DosboxExecutable '<external DOSBox-X 2026.10.01 executable>'
```

Exit code: **0**. Toolchain: Borland Turbo Pascal 5.5, DOSBox-X 2026.10.01, Node `v22.19.0`, and Python `3.13.7`.

- Direct Pascal sessions: **148/148** (54 standard probes; 94 standalone/fixture sessions).
- Configured direct Pascal-to-production comparisons: **68/68** (65 existing, signed `.BRD`, signed `.SAV`, and short-string `.BRD`).
- Integrated replay: **F01–F10, 10/10**, 238 checkpoints, 824,821 state-field comparisons, and 169 logical-cycle snapshots.
- Unexplained comparable mismatches: **0**.

This adds D/DP evidence to the existing frozen E2-02 AC002 record. AC002 remains partial: Boolean variants, reserved tails, other stale tails, `DataLen`, board/stat count edges, RLE boundaries, and applicable malformed/truncated inputs remain unverified. No acceptance IDs or mandatory scope were added.
