# Verification record — 2026-11-24

This is the latest clean compatibility run for the current E2-02 editor-transfer changes. It does not close an Engine 2.0 blocker.

## Direct Pascal and differential run

- Command: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain <Turbo Pascal 5.5 directory> -DosboxExecutable <DOSBox-X 2026.10.01 executable>`.
- Working source checkpoint: local root Git revision `1c4d5ef` (`fix: share Pascal .BRD editor transfer semantics (E2-02 partial)`); reconstructed Pascal gameplay sources were unchanged.
- The runner recreated `build/pascal-compat-verification` from the reconstructed source and controlled fixtures.
- Exit code: **0**. **84 direct Turbo Pascal 5.5 sessions passed**: 54 standard programs and 30 standalone sessions.
- **43/43 configured Pascal-to-production comparisons passed**, including the short-payload editor transfer outcome and seven selected board/player fields.
- The shared `.HI` suite passed **16 direct byte comparisons** against compiled Pascal: one save, eight load/reset, and seven add cases, each comparing the complete 1,590-byte table. The separate 256-row insertion model remains source-derived evidence.
- The integrated prototype compared **30,789 fields**: 3,421 fields at each of nine checkpoints (loaded state, game-loop start, six cycles, and final state). RNG seed words, one stat script-pointer alias relation, and the fixture script bytes were included. All matched. It is not one of the ten E2-10 fixtures and still lacks complete alias graphs, cell-write and sound-event streams, state-transition checkpoints, and generated file bytes.
- Source-derived evidence remains separate: 256 `.HI` insertion model rows and five Android CP437 name-adapter assertions are not counted as compiled-Pascal probes.
- The valid short-header fixture attempt (`SHORTHD.BRD`) is unresolved: the DOSBox-X run stalled; an instrumented run ended with Pascal runtime error 203 in the freed-buffer `BoardOpen` path. It is not included in the 84 passing sessions or 43 passing comparisons. The older nine-character `SHORTHEAD` fixture was invalid under DOS 8.3 and its earlier purported output was removed.

## Other suites and builds

| Command | Result | Notes |
|---|---|---|
| `tools/serializer-compat/verify.ps1` | Exit 0 | Shared `.BRD` codec, world/save round trips, RLE, truncation behavior, and editor short-payload state harness. |
| High-score checks invoked by the clean Pascal runner | Exit 0 | 16 direct Pascal byte comparisons, 256 source-model rows, and five Android CP437 adapter assertions. |
| Runtime checks invoked by the clean Pascal runner | Exit 0 | Production runtime harness passed; its production-only cases remain incompletely enumerated in the frozen inventory. |
| Android checks invoked by the clean Pascal runner | Exit 0 | CP437 bytes, colors/blink/monochrome rules, and DOS-key mapping. |
| `.\gradlew.bat --offline :app:assembleDebug :app:assembleDebugAndroidTest` with `GRADLE_USER_HOME=build\gradle-home` | Exit 0 | 64 tasks (13 executed, 51 up-to-date); native engine built for `arm64-v8a` and `x86_64`. No live-device claim. |
| Windows x64 Release / CTest | Not reverified in this run | A CMake/MSBuild attempt remained idle at `Checking File Globs` and was interrupted. The earlier 3/3 CTest pass predates this E2-02 editor-transfer API change and is not counted as current evidence. |

## Toolchain identity

Turbo Pascal and DOSBox-X binaries are excluded from the repository. The recorded local inputs were Turbo Pascal 5.5 and DOSBox-X 2026.10.01. The DOSBox-X executable SHA-256 was `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`. Turbo Pascal hashes are recorded below for repeatability without redistributing the tools.

| Local tool input | SHA-256 |
|---|---|
| `TPC.EXE` | `3155758683FC993653AD7076A0FA2B7D269D8838F4725F424B791F43DC56A909` |
| `TURBO.TPL` | `98D24B53817139CB83B7F4ED2723DE103772AF93A43C9E1E9D8D6D01298B6949` |
| DOSBox-X executable | `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3` |

The Android build used the existing Gradle 8.11.1 distribution, Android Gradle Plugin 8.10.0, SDK Platform 35, and NDK 27.2.12479018. Node.js was `v22.19.0`; Java was 17. Local paths and proprietary compiler/emulator binaries are intentionally omitted.

## Result and limits

No unexplained mismatch occurred in the 43 configured differential comparisons. This statement is scoped to those finite configured cases. E2-02 remains partial because short-header direct editor state is unresolved and the frozen `.BRD` malformed-read matrix is incomplete. E2-10 remains open at **0/10** required integrated fixtures. Known discrepancies and remaining mandatory work are listed in [`ENGINE_2.0_CHECKLIST.md`](../../../ENGINE_2.0_CHECKLIST.md) and [`COMPATIBILITY_PROGRESS.md`](../../../COMPATIBILITY_PROGRESS.md).
