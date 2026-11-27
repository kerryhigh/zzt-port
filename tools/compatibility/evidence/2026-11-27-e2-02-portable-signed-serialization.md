# E2-02 signed serialization portability verification

Date: 2026-11-27  
Engine source revision: `e2bbf2d45f535c03b532b309ae6f2eaefd30d6d8`  
LibZoo local provenance: `e5d2fe7301828a360e467e20ffd88b6c759db5f5`  
Evidence classification for the exhaustive codec check: **P**, with its expected two's-complement mapping derived from the Pascal record layout. The full oracle run below is separately classified **D/DP/I** by its existing probes and fixture records.

## Change and regression

The shared serializer now maps raw Pascal `Integer` and `ShortInt` bit patterns through range-safe intermediate values. Signed writes first convert to `uint16_t`, then emit the same low and high bytes as before. The `.BRD` reader uses the same defined 16-bit interpretation. Partial world board-length reads still preserve the unread prior byte before interpreting the resulting word.

`tools/serializer-compat/verify.ps1` now runs an exhaustive production regression through the shared codec: all 65,536 word patterns pass through the world serializer writer/reader and `.BRD` signed-field decoder; all 256 byte patterns pass through packed signed-byte interpretation. The test also checks every serialized low/high byte and complete input/output cursor positions. Result: pass, exit 0. The existing fixture checks also passed, including full `.ZZT`/`.SAV` round-trips, all 16 `.BRD` records, malformed-length behavior, strings, Boolean bytes, reserved bytes, RLE, and stat-data aliasing.

This is production regression evidence, not a new Pascal differential and not a new acceptance case. It resolves the implementation-defined C conversions identified within the existing signed-field boundary requirement. E2-02 AC002 and the E2-02 blocker remain partial until their required direct compiled-Pascal and production differentials cover the remaining matrix.

## Clean compatibility run

Command (DOSBox-X executable path intentionally redacted):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain 'build/pascal-compat-run/Z' -DosboxExecutable '<external DOSBox-X 2026.10.01 executable>'
```

Exit code: **0**. Borland Turbo Pascal 5.5 ran through DOSBox-X 2026.10.01; the external executable SHA-256 was `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`. The proprietary compiler and emulator binaries remain outside the repository.

- Direct compiled-Pascal sessions: **148/148** (54 standard probes, 94 standalone/fixture sessions).
- Configured direct Pascal/production differential invocations: **65/65** passed.
- Frozen integrated corpus: **F01–F10, 10/10** passed; 238 checkpoints, 824,821 state-field comparisons, and 169 logical-cycle snapshots; no unexplained comparable mismatch.
- E2-10 F10 malformed-load state: 3,385/3,385 comparable fields matched; private `World.BoardLen[0]` remains excluded as documented.
- Runtime compatibility suite: `tools/runtime-compat/verify.ps1`, exit 0.
- Windows x64 build and CTest: `windows/build.ps1`, exit 0; 3/3 tests passed.
- Android CP437/input bridge: `tools/android-compat/verify.ps1`, exit 0.
- Android offline debug and instrumentation-test APK build: `:app:assembleDebug :app:assembleDebugAndroidTest`, exit 0; native libraries built for `arm64-v8a` and `x86_64`.
- Physical Android device verification was not performed.

The exact commands, exit codes, and current summary are recorded in `tools/compatibility/latest_verification.json`; `COMPATIBILITY_PROGRESS.md` is generated from that run record and the frozen acceptance inventory.
