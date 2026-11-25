# ZZT Engine Port

This repository contains a shared C implementation of the original ZZT game engine, an Android frontend, a native Windows frontend, and a reconstructed Pascal reference tree with compatibility probes. The project preserves the observable behavior of DOS ZZT, including file-format oddities and gameplay bugs.

## Engine 2.0 compatibility milestone

The LibZoo implementation at [`7fb3e89a8467e66a4b8cbc290c708b9fa5290886`](third_party/libzoo) is the project's version 1.0 baseline. Engine 2.0 is a finite verification milestone for a stable, platform-independent engine. It does not authorize gameplay changes or new features. The release blockers and acceptance rules are in [`ENGINE_2.0_CHECKLIST.md`](ENGINE_2.0_CHECKLIST.md); live counts and latest evidence are in [`COMPATIBILITY_PROGRESS.md`](COMPATIBILITY_PROGRESS.md).

Engine 2.0 covers shared world and board state, gameplay timing, element and object behavior, ZZT-OOP, file semantics, editor mutations, high-score behavior, logical text-cell writes, and logical sound events. Windows and Android presentation, device adapters, rendering, physical input, audio output, packaging, and distribution are outside that milestone.

## Current status

The shared engine is used by both frontends. The last completed full Pascal run passed 84 Turbo Pascal 5.5 probe sessions and 43 configured Pascal/production differential invocations; the shared `.HI` table also passed 16 direct Pascal byte comparisons. A newer full-run attempt reached the integrated gameplay probe but timed out in the subsequent `BRDXFER` DOSBox-X session; its separate integrated differential did pass. The `.BRD` short-payload editor transfer matches the direct Pascal outcome and seven selected board/player fields. A production-only test also verifies the deterministic safe-reopen model for a one-byte header, but the corrected Pascal short-header probe remains unresolved because the freed-buffer `BoardOpen` path stalls or reports runtime error 203. The integrated prototype matched 31,242 variable-size state fields across loaded state, loop start, six cycle checkpoints, and final state, plus two sound queue events (20 event fields). It includes RNG seed words, the complete 3×3 script-pointer equality matrix, both unique scripts, and sound queue state. It remains separate from the ten required fixtures. Engine 2.0 remains **in progress** with **0/11 blockers closed** and the ten-fixture corpus outstanding. This repository does not claim complete ZZT compatibility.

The Android app keeps its portrait 60×25 playfield. The Windows frontend presents the DOS 80×25 character grid and supports windowed/fullscreen use. After the shared editor-transfer API change, the Windows x64 engine, frontend, smoke, renderer, and PCM targets rebuilt from the generated Visual Studio projects and CTest passed 3/3. Android debug and instrumentation APK builds passed fresh for arm64-v8a and x86_64. Platform implementation checklists are [`PORT_CHECKLIST.md`](PORT_CHECKLIST.md) and [`WINDOWS_PORT_CHECKLIST.md`](WINDOWS_PORT_CHECKLIST.md); their UI and device requirements do not gate Engine 2.0.

## Build

### Android

Install JDK 17, Android SDK Platform 35, and Android NDK 27.2.12479018. The checked-in Gradle wrapper uses Gradle 8.11.1 and Android Gradle Plugin 8.10.0. From the repository root:

```powershell
./gradlew :app:assembleDebug
```

The debug APK is written under `app/build/outputs/apk/debug/`. Android Studio can also open this Gradle project. A connected Android device is required for live device checks; a successful build is not device validation.

### Native Windows

On 64-bit Windows, install Visual Studio 2022 C++ build tools, CMake 3.22 or later, and Python 3:

```powershell
cmake -S windows -B build/windows-x64 -G "Visual Studio 17 2022" -A x64
cmake --build build/windows-x64 --config Release
ctest --test-dir build/windows-x64 -C Release --output-on-failure
```

Run `build/windows-x64/Release/ZZT.exe`. CTest covers the Windows engine/I/O smoke path, renderer scaling, and logical PCM timing; it does not prove full DOS compatibility.

## Compatibility verification

Run the platform-independent production and serializer checks from PowerShell:

```powershell
./tools/serializer-compat/verify.ps1
./tools/highscore-compat/verify.ps1
./tools/runtime-compat/verify.ps1
./tools/android-compat/verify.ps1
```

The shared high-score harness also requires Node.js, Java 17, and the Android NDK used by this project; it compiles the actual LibZoo C implementation to WASM and separately checks Android CP437 name encoding. The direct Pascal suite needs a private local Turbo Pascal 5.5 installation containing `TPC.EXE` and `TURBO.TPL`, plus DOSBox-X. Those proprietary toolchain binaries are intentionally excluded from this repository. Configure a local copy of Turbo Pascal under `build/pascal-compat-run/Z` or pass `-PascalToolchain`; then run:

```powershell
./tools/pascal-compat/verify.ps1 -DosboxExecutable "<path-to-dosbox-x.exe>"
```

The reconstructed source documents Turbo Pascal 5.5 acquisition and build requirements in [`third_party/reconstruction-of-zzt/README.MD`](third_party/reconstruction-of-zzt/README.MD). Node.js is required for JavaScript differential comparators. Verification staging, generated objects, APKs, local compiler binaries, and caches are ignored by Git. Turbo Pascal 5.5 must be obtained separately and configured locally; do not add its executables to commits.

Evidence labels used in the checklist and reports:

- **D** — direct probe of compiled Pascal source, with environmental hooks recorded.
- **S** — expectation derived from Pascal source but not executed by the compiled oracle.
- **P** — production-only regression with no matching Pascal output.
- **DP** — direct Pascal output compared with production output.
- **I** — integrated gameplay replay from shared initial state through named cycle and transition checkpoints.

These categories are reported separately; focused probe counts are not a substitute for integrated gameplay coverage.

## Source and licensing

Project-specific original code is licensed under the root [MIT license](LICENSE). The LibZoo baseline and reconstructed Pascal source retain their own MIT notices and the reconstruction's Epic MegaGames attribution; see [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) and the license files under `third_party/`. The reconstruction repository identifies `TOOLS/LZEXE.EXE` and `TOOLS/LZEXE.DOC` as separately licensed; they are excluded from publication. Turbo Pascal and DOSBox-X executables are external verification dependencies and are not bundled.

Upstream sources:

- [LibZoo](https://github.com/asiekierka/libzoo), baseline commit `7fb3e89a8467e66a4b8cbc290c708b9fa5290886`.
- [Reconstruction of ZZT](https://github.com/asiekierka/reconstruction-of-zzt), pinned in [`PORT_CHECKLIST.md`](PORT_CHECKLIST.md).

## Known gaps

The high-score table's initialization, load/reset, save, ranking, and insertion semantics now live in shared LibZoo; Android keeps file access and CP437 name encoding in its frontend adapter. Broader editor mutations still need shared implementation and direct comparison, and the frozen routine-to-engine/test inventory still has unclassified source entries. The integrated prototype checks selected fields at nine checkpoints, including RNG seed words and this fixture's script bytes and pointer relation. Complete script alias graphs, logical cell-write and sound-event streams, state-transition checkpoints, and generated file bytes remain open under E2-10; all ten required fixtures remain outstanding. Physical Android device testing is also pending, but is outside Engine 2.0's platform-independent acceptance scope.
