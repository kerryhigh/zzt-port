# Native Windows ZZT

This frontend builds a standalone Windows executable around the shared `third_party/libzoo` engine. It does not launch DOSBox or ship the original ZZT executable. The Android app remains a separate portrait frontend that shows only the 60×25 playfield.

## Build and smoke test

Requirements: Windows 10 or later, Visual Studio 2022 C++ tools with the x64 build tools, CMake 3.22 or later, and Python 3. The CMake project compiles the shared C engine and generates the OOP token table from the checked-in source.

From the repository root, run:

```powershell
cmake -S windows -B build/windows-stage1 -G "Visual Studio 17 2022" -A x64
cmake --build build/windows-stage1 --config Release --target zzt_windows --parallel 4
cmake --build build/windows-stage1 --config Release --target zzt_windows_smoke --parallel 4
cmake --build build/windows-stage1 --config Release --target zzt_renderer_test --parallel 4
cmake --build build/windows-stage1 --config Release --target zzt_sound_pcm_test --parallel 4
ctest --test-dir build/windows-stage1 -C Release --output-on-failure
```

Or run `powershell -ExecutionPolicy Bypass -File windows/build.ps1` to perform the same configure, target builds, and CTest run. The executable and copied runtime files are in `build/windows-stage1/Release/`. Pass `-BuildDirectory build/windows-sharp-bilinear` to build into a separate directory while another `ZZT.exe` is open.

## Run

Run `build\windows-stage1\Release\ZZT.exe` to load the bundled `DEMO.ZZT`, or pass a `.ZZT` or `.SAV` file path as its first argument. Startup looks for the default demo in the current working directory, then beside the executable. The in-game world and saved-game selectors list the current working directory only; launching from the Release folder lists the bundled demo, while launching with another working directory can show an empty selector. The file driver is writable and uses the shared DOS-compatible binary serialization.

The frontend maintains an 80×25 character-and-attribute buffer and rasterizes it directly to its native 640×350 pixel framebuffer with the bundled 8×14 CP437 font and standard 16-color DOS palette. It preserves those native glyph pixels until the final output scale. For each windowed or fullscreen frame, the renderer samples the largest virtual integer nearest-neighbor enlargement that fits on each axis independently, then applies one bilinear resize for the remaining fractional enlargement and the DOS display's non-square pixel correction. This keeps scanlines crisp when one axis has room for more integer replication than the other; at the default 960×720 viewport it uses 1× horizontally and 2× vertically before the final 1.5×/1.03× correction. Per-axis source lookups are cached before the pixel pass, avoiding repeated integer divisions without changing the virtual nearest-neighbor result. Neither integer stage is shrunk before filtering, avoiding extra softness from rounding a fractional scale upward. Exact integer scales stay pixel-crisp; fractional and aspect-correction filtering occurs only in the final pass. No GDI filtering is used. Letterboxing remains black. Alt+Enter switches to borderless fullscreen on the monitor containing the window and returns to the saved window placement when pressed again. Text-mode blink attributes are animated. Audio is sent through WinMM from the shared PCM sound driver.

Arrow keys and the numeric keypad feed shared gameplay actions; Shift requests shooting; T feeds the torch action; F1 opens libzoo's current generic menu. The title screen currently handles P (play), W (world selector), R (restore selector), S (the DOS `F....:....S` game-speed slider; digits 1–9 select a value and arrows adjust it), A (About), and Q/Escape (DOS-style quit confirmation). During play, P pauses, B toggles sound, S opens the shared save prompt, H opens `GAME.HLP`, F opens `ORDER.HLP` if present beside the executable, and Q/Escape opens the DOS-style end-game confirmation. These are an initial command slice, not a complete reimplementation of the Pascal title/play loops. High scores, editor entry and the editor, exact file prompt semantics, and `.BRD` import/export remain unimplemented.

## Current compatibility evidence and limits

The shared PCM regression checks that Windows starts each Pascal drum at its first frequency without a silent lead, changes drum frequencies at one-millisecond boundaries, and retains the shortest same-tick 110 Hz `Sound; NoSound` pulse. This covers generated PCM timing; physical speaker/audio-device playback remains a manual check.

`zzt_windows_smoke` verifies that shared redraw code writes each of the 2,000 cells of an 80×25 frame without out-of-bounds writes, loads `DEMO.ZZT` using the Win32 file driver, and preserves a generated `.SAV` byte-for-byte through load/save. `zzt_renderer_test` checks exact integer nearest-neighbor expansion, fractional bilinear edge interpolation, that a 1.5× target uses a fitting 1× integer stage rather than shrinking a 2× stage, that a 2.5× target keeps an exact 2× nearest-neighbor stage before the final fractional pass, and that the optimized virtual intermediate matches an explicitly materialized nearest-neighbor-then-bilinear reference across the complete 640×350-to-960×720 raster. It also checks smooth scanline correction from 640×350 into 4:3, the default 960×720 window case with 1×/2× per-axis integer stages, and a 1440×1080 4:3 fullscreen viewport with 2×/3× stages. Centered 4:3 fitting is tested in widescreen and portrait client areas, plus the full-size 960×720 window viewport. The fixture's `.SAV` also matched output from the reconstructed Turbo Pascal 5.5 program byte-for-byte in the development comparison.

The native window has been launched and visually checked. Live checks confirmed the 80×25 title/sidebar, speed-slider entry and digit selection, P entering play, Q confirmation (N resumes, Y returns to title), About text, F1 menu restoration, Alt+Enter fullscreen at centered 4:3 on a widescreen monitor, return to the prior window, and no black screen blanking after the back-buffer fix. These checks do not establish exact screen attributes, all menus, full game behavior, sound parity, `.HI`/`.BRD` support through the frontend, or visual accuracy on every Windows monitor. CTest remains a bounded smoke test. Window resizing, multi-monitor behavior, audio, broad keyboard cases, and Pascal/DOS reference comparisons remain open. See the checklist for the current completion gates.
