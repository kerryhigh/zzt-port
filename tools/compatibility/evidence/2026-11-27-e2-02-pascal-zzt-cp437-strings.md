# Direct Pascal `.ZZT` CP437 WorldSave strings

- Evidence class: **D+DP**.
- Acceptance case: `E2-02-AC002` (partial; no new acceptance ID or fixture-corpus completion credit).
- Reconstructed source operation: `WORLDSAV.PAS` loads the existing `DEMO.ZZT`, assigns `Chr($80)+Chr($82)+Chr($FF)` to `World.Info.Name`, `World.Info.Flags[1]`, `Board.Name`, and `Board.Info.Message`, sets `World.Info.IsSave := false`, and calls the original `WorldSave` routine to create `CP437.ZZT`.
- Production operation: `world_roundtrip.wasm` reads the Pascal `.ZZT`, opens and closes its current board, and writes the complete world through the shared LibZoo serializer.
- Engine source revision: `b4d6b8c9b3c7769d7b6be52ce61b6393a0b1eac5` (shared gameplay and serializer code unchanged for this case).
- Oracle toolchain: Borland Turbo Pascal 5.5 and DOSBox-X 2026.10.01; comparison runner Node.js 22.19.0.
- Clean command: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain 'build/pascal-compat-run/Z' -DosboxExecutable '<external DOSBox-X executable>'; exit 0`.
- Clean full-suite result: exit 0; 149 direct Pascal sessions, 74/74 configured direct differentials, F01-F10 at 238 checkpoints / 824,821 fields / 169 logical-cycle snapshots, and no unexplained comparable mismatch.

The production comparator validates the DOS `-1` world marker, `IsSave=0`, the four declared short-string lengths and raw payloads, the current board record boundaries, and the full production read/open/close/write result. Each of the four fields contained bytes `80 82 FF`. The generated Pascal file was 26,465 bytes, SHA-256 `d514554b5462757bcae3d4ceaa6a002cc4d8ec6779ee403dca41d4ca7b8e1f96`; all **26,465/26,465 bytes** matched production.

The clean verifier stages the untouched reconstructed Pascal files, compiles the existing `WORLDSAV.PAS` probe, and runs it under DOSBox-X. The only intervention for this case is the explicit deterministic field setup in that probe; no Pascal gameplay or serialization semantics are replaced. The production comparator reads the generated staging fixture and calls the shared production round-trip API. The fixture is generated during verification and is not committed as a replacement world asset.

This adds direct coverage for the `.ZZT` world string path. It does not cover every possible CP437 byte in every serialized field or in `.SAV`, and does not close AC002; the remaining frozen format, malformed-input, RLE, and untouched-buffer matrix remains open.
