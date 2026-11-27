# ZZT Engine Port

This repository contains a shared C implementation of the original ZZT game engine, an Android frontend, a native Windows frontend, and a reconstructed Pascal reference tree with compatibility probes. The project preserves the observable behavior of DOS ZZT, including file-format oddities and gameplay bugs.

## Engine 2.0 compatibility milestone

The LibZoo implementation at [`7fb3e89a8467e66a4b8cbc290c708b9fa5290886`](third_party/libzoo) is the project's version 1.0 baseline. Engine 2.0 is a finite verification milestone for a stable, platform-independent engine. It does not authorize gameplay changes or new features. The release blockers and acceptance rules are in [`ENGINE_2.0_CHECKLIST.md`](ENGINE_2.0_CHECKLIST.md); live counts and latest evidence are in [`COMPATIBILITY_PROGRESS.md`](COMPATIBILITY_PROGRESS.md). The fixed ten-scenario E2-10 corpus is recorded in [`integrated_replay_fixture_manifest.json`](tools/compatibility/integrated_replay_fixture_manifest.json).

Engine 2.0 covers shared world and board state, gameplay timing, element and object behavior, ZZT-OOP, file semantics, editor mutations, high-score behavior, logical text-cell writes, and logical sound events. Windows and Android presentation, device adapters, rendering, physical input, audio output, packaging, and distribution are outside that milestone.

## Current status

The shared engine is used by both frontends. The latest clean Turbo Pascal 5.5/DOSBox-X run dated 2026-11-26 passed 146 direct sessions (54 standard, 92 standalone) and 64/64 configured Pascal/production differential invocations. Six direct Pascal board-edge routes matched at 60/60 fields through actual production BoardChange, serialized BoardClose/BoardOpen records, call-stack continuation, and BoardEnter. The separate callback-boundary check under deterministic service stubs also matches 60/60; external world-file operations and the broader lifecycle matrix remain open. F01–F10 passed at 238 checkpoints / 824,821 state-field comparisons / 169 logical-cycle snapshots. Original `GameTitleLoop` ordinary P, successful saved-world P reload, and successful R routes matched through one real paused gameplay cycle and return-to-title (10,242; 10,360; and 10,287 fields). Unsupported-version R refusal matched 3,385/3,386 comparable state fields, and R cancel-then-retry matched four checkpoints / 13,716 fields using the same Pascal-generated `.SAV`. Direct abort decisions matched 7/7 cases, title quit decisions matched 3/3, the accepted player-abort loop boundary matched 22/22 captured fields, and refused No/Escape aftermath matched 44/44 fields. The full P -> refused abort -> zero-speed tick advance -> accepted abort -> return-to-title route matched 10,242 fields at three checkpoints. The failed backing-world title-P route also matched Pascal refusal against production `zoo_world_play` returning `ZOO_ERROR_INVAL (-2)`, with all 18 preserved world/board fields equal. E2-10 is closed at 5/5; E2-03 remains partial with 2/4 acceptance cases complete while other save/return outcomes and the broader successful/refused transition matrix remain open. Engine 2.0 remains in progress at 1/11 blockers closed across the frozen 54-case inventory. Windows x64 build and CTest passed 3/3, and Android adapter compatibility plus offline debug/instrumentation APK builds passed on 2026-11-26. These builds do not claim complete ZZT compatibility or physical-device validation.

The Android app keeps its portrait 60×25 playfield. The native Windows frontend presents the DOS 80×25 grid, but its available command and editor coverage is still partial; consult the platform checklists for those remaining frontend requirements. The 2026-11-26 shared-engine regression pass rebuilt the Windows x64 frontend, smoke, renderer, and PCM targets and passed CTest 3/3; Android CP437/key verification and the offline debug/instrumentation APK build also passed. No physical Android device check was performed in this pass. Platform UI, device, and distribution requirements do not gate Engine 2.0.

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

For each frozen acceptance case, `required_evidence_classes` lists the evidence types that case needs; this is a plan and does not imply completion. `evidence_class` records evidence attached to the case so far. The progress report shows missing evidence classifications and required-class mappings separately.

## Source and licensing

Project-specific original code is licensed under the root [MIT license](LICENSE). The LibZoo baseline and reconstructed Pascal source retain their own MIT notices and the reconstruction's Epic MegaGames attribution; see [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) and the license files under `third_party/`. The reconstruction repository identifies `TOOLS/LZEXE.EXE` and `TOOLS/LZEXE.DOC` as separately licensed; they are excluded from publication. Turbo Pascal and DOSBox-X executables are external verification dependencies and are not bundled.

Upstream sources:

- [LibZoo](https://github.com/asiekierka/libzoo), baseline commit `7fb3e89a8467e66a4b8cbc290c708b9fa5290886`.
- [Reconstruction of ZZT](https://github.com/asiekierka/reconstruction-of-zzt), pinned in [`PORT_CHECKLIST.md`](PORT_CHECKLIST.md).

## Known gaps

The high-score table's initialization, load/reset, save, ranking, and insertion semantics now live in shared LibZoo; Android keeps file access and CP437 name encoding in its frontend adapter. A direct Pascal `BoardClose`/`WorldSave` probe produces a full `.ZZT` file that matches shared LibZoo's complete world output byte-for-byte (25,928 bytes); its 289-byte board record separately round-trips through shared LibZoo. This is one direct save scenario; the four-format field/error matrix remains open. Short-header `.BRD` editor behavior still lacks a stable direct Pascal result under controlled allocation history. Broader editor mutations still need shared implementation and direct comparison, and 1,464 frozen source-inventory entries remain unclassified. E2-10's ten integrated fixtures are complete; E2-01–E2-09 and the E2-11 clean release gate remain open. Physical Android device testing is outside Engine 2.0's platform-independent acceptance scope.
