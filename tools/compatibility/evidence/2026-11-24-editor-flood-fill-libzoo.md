# Shared editor flood-fill verification — 2026-11-24

Source checkpoint: `17d9374` (`engine: share editor flood fill semantics (E2-01/E2-09 partial)`).

## Results

- Clean Pascal compatibility command: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain build/pascal-compat-run/Z -DosboxExecutable <external DOSBox-X 2026.10.01>`; exit code 0. Turbo Pascal 5.5 produced 83 passing direct probe sessions; all 42 configured Pascal/production differential comparisons passed.
- Flood-fill differential: `EDITORFL`, `EDFLDOVR`, `EDFLDQ17`, and `EDFLDFUL` maps match the compiled Pascal outputs using the production `zoo_editor_flood_fill` implementation; `EDPLACE` and `EDSTAT` placement/copy cases also pass through the Android adapter.
- Runtime suite: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/runtime-compat/verify.ps1`; exit code 0. It compiles the shared LibZoo source and calls it through the extracted production JNI wrapper and placement callback.
- Android: `.\gradlew.bat --offline :app:assembleDebug :app:assembleDebugAndroidTest`; exit code 0, 64 tasks (7 executed, 57 up-to-date), arm64-v8a and x86_64.
- Windows: clean x64 Release configuration with CMake 3.31.6, MSVC 19.44.35215, Windows SDK 10.0.26100; `ZZT.exe` and three test targets built. CTest: 3/3 passed (engine/I-O and 80×25 smoke, sharp-bilinear renderer, PCM timing).
- DOSBox-X was version 2026.10.01 (SHA-256 `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`). Turbo Pascal and DOSBox-X binaries remain external dependencies.

## Scope still open

E2-01 and E2-09 remain open. Flood-fill traversal is shared and directly differentially verified, while placement/paste/stat mutation side effects and high-score operations remain outside the shared engine. The integrated E2-10 corpus is still 0/10; this focused editor change does not close any release blocker.
