# Direct Pascal `.SAV` and `.ZZT` embedded-NUL/CP437 WorldSave strings

- Evidence class: **D+DP**.
- Acceptance case: `E2-02-AC002` (partial; no new acceptance ID or fixture-corpus completion credit).
- Reconstructed source operation: `WORLDSAV.PAS` loads the existing `DEMO.ZZT`, assigns `Chr(0)+Chr($80)+Chr($82)+Chr($FF)` to `World.Info.Name`, every `World.Info.Flags[1..10]` entry, `Board.Name`, and `Board.Info.Message`, then calls the original `WorldSave` routine once with `IsSave := false` for `CP437.ZZT` and once with `IsSave := true` for `CP437.SAV`.
- Production operation: `world_roundtrip.wasm` reads each Pascal file, opens and closes its current board, and writes the complete world through the shared LibZoo serializer.
- Engine source revision: `b4d6b8c9b3c7769d7b6be52ce61b6393a0b1eac5` (shared gameplay and serializer code unchanged for this case).
- Oracle toolchain: Borland Turbo Pascal 5.5 and DOSBox-X 2026.10.01; comparison runner Node.js 22.19.0.
- Clean command: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain 'build/pascal-compat-run/Z' -DosboxExecutable '<external DOSBox-X executable>'; exit 0`.
- Clean full-suite result: exit 0; 149 direct Pascal sessions, 74/74 configured direct differentials, F01-F10 at 238 checkpoints / 824,821 fields / 169 logical-cycle snapshots, and no unexplained comparable mismatch.

The production comparator validates the DOS `-1` world marker, the appropriate `IsSave` byte for each format, the 13 declared short-string lengths and raw payloads (world name, ten flags, current board name, and message), current board record boundaries, and each complete production read/open/close/write result. All 13 fields in each file contained bytes `00 80 82 FF`, including an embedded NUL. Both generated Pascal files were 26,465 bytes and matched byte-for-byte:

| File | IsSave | Matching bytes | SHA-256 |
|---|---:|---:|---|
| `CP437.ZZT` | 0 | 26,465/26,465 | `47c5a657b5c4fb9c04e99ac4fd8a623c79870cd5db9aaf3ba37f0f34ff33cdfa` |
| `CP437.SAV` | 1 | 26,465/26,465 | `1eddbe003459ab80ff88730f070d03cc4d148b1d7c5be7a0bbe007f427f345d7` |

The 26 string-field instances and **52,930/52,930 file bytes** matched production.

The clean verifier stages the untouched reconstructed Pascal files, compiles the existing `WORLDSAV.PAS` probe, and runs it under DOSBox-X. The only intervention for this case is the explicit deterministic field setup in that probe; no Pascal gameplay or serialization semantics are replaced. The production comparator reads the generated staging fixture and calls the shared production round-trip API. The fixture is generated during verification and is not committed as a replacement world asset.

This adds direct coverage for embedded-NUL/CP437 values in the `.SAV` and `.ZZT` WorldSave paths, including all ten flags. It does not cover every possible short-string length or every serialized field/format, and does not close AC002; the remaining frozen format, malformed-input, RLE, and untouched-buffer matrix remains open.
