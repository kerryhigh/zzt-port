# E2-03-AC003 — integrated logical-cycle snapshots

**Result: passed for the frozen per-cycle acceptance case.** E2-03 remains partial; lifecycle transition coverage and the final transition/schedule pass gate are still outstanding.

## Compared state

The direct Pascal replay and shared production `zoo_tick` replay compare every recorded checkpoint across the frozen F01–F10 corpus. Each logical-cycle snapshot contains:

- World name length/bytes, current board number/name, and all board tiles.
- Tick, stat tick, tick speed, tick duration/counter, board elapsed time, and paused/exit state.
- All stat fields and owned script bytes; the message timer is represented by its real stat slot.
- Player resources, position, movement state, input deltas, key value, and buffered-key count.
- RNG state, script-pointer alias relationships, sound queue fields/bytes, and cumulative sound-event count.
- Cumulative effective cell-write count. The ordered cell stream is then compared as `(x, y, attribute, CP437 byte)` records with surrounding production context on divergence.

The Pascal clock and normalized gameplay inputs are scripted. Wall-clock polling is not compared as a compatibility value.

## Verification

- Clean Pascal compatibility runner: exit 0; 126/126 direct Turbo Pascal 5.5 sessions; 54/54 configured Pascal-to-production comparisons.
- Frozen integrated corpus: F01–F10 passed, 238 checkpoints and 824,821 compared state fields, including 169 logical-cycle checkpoints. No state-field, sound-event, effective-cell-write, high-score, or stable-serialization differences were reported.
- Across the fixtures, 17 logical sound events and 16,470 ordered effective playfield writes matched. The 169 cycle checkpoints compared their cumulative cell-write counts, so per-cycle write deltas matched as well.
- The first-divergence negative control still reports its injected ammo mismatch, while confirming the original Pascal output SHA-256 is unchanged.
- Standalone runtime compatibility suite: exit 0.

## Known transition-boundary observation

In F05, the cumulative effective cell-write counter at state-transition checkpoint T3 is Pascal 6003 versus production 6004. Diagnostic continuation compared the remaining snapshots, complete ordered cell stream, sound stream, and serialized state: the only checkpoint field difference was this transition counter, and all 6,006 effective cell-write records matched in order. The extra count is the same next screen write on opposite sides of the intermediate transition snapshot; all six logical-cycle counters match. F05/F08 also retain the separately classified 834-versus-835 identical-value redraw count.

For these two fixtures the comparator therefore does not use the derived cumulative cell counter at intermediate **T** checkpoints as a pass field. It still checks the counter at every **C** logical-cycle checkpoint, checks overflow, and compares every effective write in the global ordered stream. This boundary observation remains recorded for E2-03 transition acceptance; it is not counted as an AC003 cycle mismatch or hidden as a gameplay fix.

## Reproduction

```powershell
$dosbox = $env:DOSBOX_X_EXE
if (-not $dosbox) { $dosbox = (Get-Command dosbox-x.exe).Source }
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 `
  -PascalToolchain build/pascal-compat-run/Z `
  -DosboxExecutable $dosbox
powershell -NoProfile -ExecutionPolicy Bypass -File tools/runtime-compat/verify.ps1
```

Toolchain: Borland Turbo Pascal 5.5, DOSBox-X 2026.10.01 (SHA-256 `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`), Node.js 22.19.0, and Python 3.13.7. Proprietary compiler binaries remain external and are not redistributed. Oracle instrumentation is generated in the ignored staging copy; reference gameplay semantics are not modified.
