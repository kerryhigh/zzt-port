# Native Windows ZZT completion checklist

Stage 1 is complete only when the Windows frontend preserves the original DOS behavior and the remaining manual gates have evidence. Current evidence is deliberately scoped; the port is not declared fully compatible.

## Implemented and checked

- [x] Inspect architecture and keep Windows rendering/I/O in `windows/`, reusing libzoo engine, serializer, classic sidebar, software renderer, CP437 font, and PCM driver.
- [x] Add an x64 Visual Studio/CMake target that creates a standalone `ZZT.exe` and copies demo/help runtime files beside it.
- [x] Capture the 80×25 cell grid and draw the 60×25 playfield plus 20-column sidebar through the shared renderer.
- [x] Use a DOS 16-color palette, text attributes, blinking, and the native 640×350 raster from the 80×25 8×14 CP437 font; both windowed and fullscreen modes virtually expand by the largest integer nearest-neighbor scale that fits independently on each axis, then apply bilinear filtering for the final fractional scale and 4:3 pixel-aspect correction (neither integer stage is shrunk).
- [x] Scaler regressions cover exact integer replication, fractional edge blending, a fitting 1× stage rather than shrinking a 2× stage at 1.5×, a 2× nearest-neighbor stage followed by the fractional pass, smooth 640×350 scanline correction to 4:3, and a full-raster comparison of the virtual scaler against an explicitly materialized nearest-then-bilinear reference for the default 960×720 window's mixed 1×/2× stages. They also cover a 1440×1080 fullscreen viewport's 2×/3× stages and centered 4:3 viewport fitting in widescreen, portrait, and standard window areas. Per-axis source lookups are cached before filtering to avoid repeated integer divisions without changing output.
- [x] Match the shared Windows PCM driver to Pascal drum timing: drum sequences start at their first frequency, each frequency occupies one millisecond, and a same-tick 110 Hz `Sound; NoSound` pulse survives; the new CTest covers all three boundaries.
- [~] The 2026-11-24 `windows-sharp-bilinear-final` title screen was visible at 960×720 and the unobscured CP437 strokes appeared crisp. A Windows Security dialog for an OpenJDK process covered the center; the `verify2` and current-turn executables did not expose a new targetable window in the desktop window inventory, so visual review of the latest build and full windowed/fullscreen sharpness review remain open. Existing ZZT sessions were left open.
- [x] Rebuilt `build/windows-sharp-bilinear-confirm/Release/ZZT.exe` with Visual Studio 2022 x64 on 2026-11-24 for the sharp-bilinear renderer request; all three CTests passed, including exact integer replication, fractional interpolation, windowed 640×350-to-4:3 correction, the default 960×720 mixed-axis filter, the 1440×1080 fullscreen 2×/3× stage regression, and centered letterbox geometry for widescreen and portrait areas. The native framebuffer and final scaler are shared by the windowed and fullscreen paint paths.
- [x] Freshly rebuilt `build/windows-sharp-bilinear-20261124-final-turn/Release/ZZT.exe` on 2026-11-24 from the checked renderer sources; the standalone x64 Windows build succeeded and all three CTests passed (engine/I/O, integer-plus-fractional sharp-bilinear scaling, and PCM timing). No engine, gameplay, or Android presentation sources changed.
- [x] Rebuilt `build/windows-sharp-bilinear-20261124-verify/Release/ZZT.exe` after adding the explicit materialized-pipeline comparison. The 640×350-to-960×720 synthetic raster matched the virtual nearest-then-bilinear result to within one channel value, and all three CTests passed: Win32 engine/I/O, sharp-bilinear rendering, and PCM sound timing.
- [x] Rebuilt `build/windows-renderer-review-20261124/Release/ZZT.exe` from the current source. The shared windowed/fullscreen path rendered through the 640×350 CP437 framebuffer and sharp-bilinear scaler; all three CTests passed, including integer replication, fractional filtering, materialized-pipeline comparison, 4:3 window/fullscreen scaling, engine/I/O smoke, and PCM timing.
- [x] Follow-up verification (2026-11-24): MSBuild confirmed the x64 `zzt_windows` and `zzt_renderer_test` targets in `build/windows-renderer-review-20261124` with zero warnings/errors; CTest rerun passed all three tests. The checked renderer sources are isolated under `windows/`; no Android layout or shared game-engine source was changed for this filter pass.
- [x] Rebuilt the shared x64 Windows frontend after the zero-duration scheduler correction at `build/windows-zero-speed-check/Release/ZZT.exe`; all three CTests passed for 80×25 engine/I/O, sharp-bilinear scaling, and PCM drum timing.
- [x] Add proportional window resizing, borderless current-monitor fullscreen, saved window placement, and swallowed Alt+Enter handling.
- [x] Double-buffer window presentation and redraw only when DOS blink phase changes; live screenshots after the fix showed no blank-screen flash.
- [x] Add video-cell readback so DOS-style windows can save and restore the full underlying cell region.
- [x] Add Win32 file access and a CTest smoke check for 2,000 in-range cells, loading `DEMO.ZZT`, and byte-stable `.SAV` load/save.
- [x] Check title-sidebar DOS strings and attributes in CTest, including the `ZZT` header, Play, Restore, About, High Scores, and exact default game-speed slider cells.
- [x] Compare the smoke fixture's 26,465-byte `.SAV` output with the reconstructed Turbo Pascal run: byte-identical in the recorded comparison.
- [x] Add reproducible build instructions and `windows/build.ps1`.
- [x] Correct the classic sidebar header to match Pascal's `ZZT` text.
- [x] Build the Windows x64 Release target and pass `win32_engine_io_and_80x25` through `windows/build.ps1`.
- [x] Rebuild Android `assembleDebug` and pass `serializer-compat`, `runtime-compat`, and `highscore-compat` verifiers; no Android frontend source or layout was changed.
- [x] Wire and visually check title P, W, A, and Q/Escape commands; confirm P enters the demo board and the shared world selector/About screen open.
- [x] Wire restore R to the shared save selector and in-game S/H/F to shared save/help paths.
- [x] Visually verify Q quit confirmation: N resumes play, Y returns to the title; verify F1 menu and About windows restore the prior DOS screen.
- [x] Port the Pascal title-screen speed slider using the original `F....:....S` scale, digit 1–9 values, arrow adjustment, and `tick_speed`; check its exact title cells in CTest and entry/digit selection live.
- [x] Visually verify Alt+Enter enters centered 4:3 borderless fullscreen with side bars on the current widescreen monitor and restores the windowed display.

## Still open before Stage 1 completion

- [ ] Complete the Pascal title loop and exact DOS command dispatch, including high scores, editor entry, and all prompt edge cases; compare speed-slider repeat and shifted-arrow behavior against Pascal.
- [ ] Compare original in-game save/pause/quit/sound/help command handling, timing, key buffering, and prompt behavior against Pascal.
- [ ] Integrate the original board editor and `.BRD` import/export; verify existing `.HI` behavior and file naming.
- [ ] Compare all 2,000 screen characters and attributes against the Pascal/DOS reference for title, gameplay, dialogs, menus, and editor.
- [ ] Compare keyboard buffering, repeat cadence, simulation timing, movement/combat, OOP behavior, and audio output against the source/reference.
- [ ] Run and record manual proportional resizing, multi-monitor fullscreen, keyboard repeat/navigation, and audio checks.
- [ ] Validate `.ZZT`, `.SAV`, `.BRD`, and `.HI` load/save interoperability across representative worlds, malformed inputs, and Pascal edge cases.
- [ ] Verify world/save selector contents and path behavior in the current working directory, including spaces and DOS-style extensions; startup's executable-folder demo fallback does not change the selector directory.
- [~] Verify Android's 60×25 portrait presentation on an emulator or device. The 2026-11-23 Android 16 x86_64 emulator instrumentation exercised the app UI and passed touch/editor-surface checks; no screenshot-based layout review or physical-device visual/input validation was performed in this Windows slice.
- [ ] Document residual differences after the above comparisons.

## Known limits in this build

The current frontend handles title P/W/R/S/A/Q and in-game Q/Escape, S, H, and F commands, while F1 still opens libzoo's generic menu. High scores, editor entry and editor operations, `.BRD` UI, and full Pascal text/file prompt behavior are missing. Key-by-key DOS screen comparison and broad gameplay/audio equivalence are not present. The speed slider uses the Pascal display and value range, but its held-key timing and shifted-arrow edge cases have not been compared. The selectors enumerate the process's current working directory; the executable-folder demo fallback applies to default startup only. The 8×14 CP437 font is an explicit adapter choice; ZZT requested 80×25 text mode without loading a glyph set, so exact raster appearance depended on the original display adapter. The `.SAV` comparison covers one fixture only. Android standard `assembleDebug` and focused serializer/runtime/high-score suites passed; isolated emulator instrumentation passed on 2026-11-23 for touch, editor fills, title/gameplay visibility, and failed-open preservation. Device rendering and physical-device input remain unverified.
