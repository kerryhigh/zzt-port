# Verification record — 2026-11-24

This note records the latest clean cross-suite run and its limits. It does not close an Engine 2.0 blocker.

## Direct Pascal and differential run

- Command: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain <local Turbo Pascal 5.5> -DosboxExecutable <local DOSBox-X>`.
- Source checkpoint: `ce9f561` (`engine: share .HI table semantics (E2-01/E2-09 partial)`). Reconstructed Pascal gameplay sources were not changed.
- The runner recreated `build/pascal-compat-verification` from the reconstructed Pascal source and controlled fixture inputs.
- Exit code: **0**. The suite reported **83 direct Turbo Pascal 5.5 probe sessions**: 54 standard probe programs plus 29 standalone sessions.
- All **42** configured Pascal-to-production comparison invocations in `tools/pascal-compat/verify.ps1` completed successfully.
- The shared `.HI` suite passed **16 additional direct byte comparisons** against the compiled Pascal outputs: one save record, eight load/reset results, and seven insertion results. Each result compares the entire 1,590-byte table. The separate 256-row insertion model remains source-derived evidence.
- The integrated prototype compared **30,591 fields**: 3,399 selected world/stat/scheduler/input/tile fields at each of nine checkpoints (loaded initial state, game-loop start, six completed cycles, and final state). All matched. The comparator reports the earliest differing checkpoint and field with neighboring field values.
- The prototype uses six deterministic zero-speed cycles, but it is not one of the ten complete E2-10 fixtures. Script bytes and alias graphs, RNG state, emitted cell-write/sound-event streams, board/state transitions, and generated file bytes remain outside this prototype snapshot schema.
- Source-derived evidence remains distinct: `.HI` insertion has 256 model-generated outputs that supplement, but do not replace, the seven direct Pascal insertion scenarios. Five host-Java assertions check the Android CP437 name adapter separately.

## Other suites

| Command | Result | Notes |
|---|---|---|
| `tools/serializer-compat/verify.ps1` | Exit 0 | Shared `.BRD` production codec, world/save round trips, RLE and truncation behavior. |
| `tools/highscore-compat/verify.ps1` | Exit 0 | Shared LibZoo `.HI` table implementation; 16 direct Pascal byte comparisons, 256 source-model rows, and five Android CP437 adapter assertions. |
| `tools/runtime-compat/verify.ps1` | Exit 0 | Production runtime harness; its individual production-only regression cases are not yet enumerated in the frozen acceptance inventory. |
| `tools/android-compat/verify.ps1` | Exit 0 | CP437 bytes, colors/blink/monochrome rules, and DOS-key mapping. |
| Android Gradle debug and instrumentation builds | Passed | Offline Gradle 8.11.1 / Android Gradle Plugin 8.10.0, SDK Platform 35, NDK 27.2.12479018; arm64-v8a and x86_64. No live device claim. |
| Windows x64 Release build and CTest | Build passed; 3/3 passed | MSVC 14.44.35207, Windows SDK 10.0.26100, CMake 3.31.6; engine/I-O smoke, sharp-bilinear scaling, and PCM timing after the shared `.HI` refactor. Visual and DOS parity remain separate gates. |

## Toolchain identity

Turbo Pascal 5.5 and DOSBox-X executables are excluded from this repository. The hashes below identify the local inputs used for this run without distributing them. The DOSBox-X executable did not expose a file-version string in Windows metadata.

| Local executable/data | SHA-256 |
|---|---|
| Turbo Pascal 5.5 `TPC.EXE` | `3155758683FC993653AD7076A0FA2B7D269D8838F4725F424B791F43DC56A909` |
| Turbo Pascal 5.5 `TURBO.TPL` | `98D24B53817139CB83B7F4ED2723DE103772AF93A43C9E1E9D8D6D01298B6949` |
| DOSBox-X executable | `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3` |

The host had Node.js `v22.19.0` and Java 17. The Windows build used the toolchain versions in the table above. User-specific local paths and the raw ignored console log are intentionally omitted.

## Result

No unexplained mismatch was reported by this run. That is scoped to the configured focused comparisons; broad callback branches and all required integrated sessions are not yet covered. Known discrepancies and pending work remain listed in [`ENGINE_2.0_CHECKLIST.md`](../../../ENGINE_2.0_CHECKLIST.md) and [`COMPATIBILITY_PROGRESS.md`](../../../COMPATIBILITY_PROGRESS.md).
