# ZZT Engine 2.0 Compatibility Checklist

**Status:** acceptance checklist; Engine 2.0 is not declared complete.  
**Snapshot reviewed:** 2026-11-24.  
**Baseline:** original LibZoo implementation at `7fb3e89a8467e66a4b8cbc290c708b9fa5290886` (`2020-08-02`, upstream `asiekierka/libzoo`).

Engine 2.0 means a stable, platform-independent implementation of the original ZZT engine, with compatibility supported by direct compiled-Pascal comparisons and integrated game traces. It is a verification milestone, not permission to improve or reinterpret ZZT. Keep observable original behavior, including bugs and awkward file semantics.

This is a finite checklist. Close the blockers in [Release blockers](#release-blockers) and the final verification gate. Add a new mandatory requirement only when new evidence demonstrates a material compatibility defect; place other discoveries in [Optional future investigations](#optional-future-investigations). Do not expand this milestone for speculative completeness.

## Scope

In scope: shared world and board state, simulation/tick rules, player and object behavior, ZZT-OOP, the game-state effects of normalized input and menu decisions, deterministic clock and random behavior, byte-level serialization, editor board mutations and `.BRD` operations, high-score data/ranking behavior, engine-produced text-cell writes, text-window semantics, and logical sound events.

Out of scope: Windows or Android screens and layouts, pixel rendering or fonts, platform input adapters and physical key repeat, platform audio output, device/emulator testing, window/fullscreen behavior, packaging, installation, and distribution. When an engine routine produces cells or events, compare those logical outputs; do not require a particular device raster or audible waveform. Menu/prompt presentation is excluded, but a shared decision that changes engine state remains in scope.

## Baseline and audit limits

- The nested LibZoo repository's recorded upstream baseline is `7fb3e89a8467e66a4b8cbc290c708b9fa5290886`. The current checkout contains a substantial local engine overlay in `third_party/libzoo` (including `zoo_game.c`, `zoo_game_io.c`, `zoo_elements.c`, `zoo_oop.c`, `zoo_input.c`, `zoo_sound.c`, `zoo_window.c`, `zoo_window_classic.c`, `zoo.h`, and related files). The nested worktree was modified at review time; this checklist describes the current implementation, not the pristine upstream revision.
- The reconstructed Pascal source is under `third_party/reconstruction-of-zzt/SRC` (`GAME.PAS`, `ELEMENTS.PAS`, `OOP.PAS`, `GAMEVARS.PAS`, `INPUT.PAS`, `KEYS.PAS`, `SOUNDS.PAS`, `TXTWIND.PAS`, `VIDEO.PAS`, `EDITOR.PAS`, and `ZZT.PAS`). Treat the source as the compatibility specification where the original executable is nondeterministic or unavailable.
- The root Git history was established on 2026-11-24 because the workspace had no root `.git` directory. The initial root commit preserves the actual current source tree and descends from the local LibZoo compatibility overlay; there was no earlier root history to inspect or recover. The nested LibZoo clone is shallow at baseline `7fb3e89a8467e66a4b8cbc290c708b9fa5290886`, so history before that recorded upstream commit was unavailable locally and is not fabricated. This provenance limitation is not a compatibility pass.
- Compatibility state below reflects the latest dated project records reviewed, especially `PORT_CHECKLIST.md` and `WINDOWS_PORT_CHECKLIST.md`; it is not a claim that every suite was rerun while authoring this document.

## Evidence labels

Use these labels in test names, reports, and completion notes. Never describe an assumption or a production self-test as direct Pascal verification.

| Label | Meaning | What it establishes |
|---|---|---|
| **D** | Direct compiled-Pascal probe | A probe calls the reconstructed Pascal implementation compiled by Turbo Pascal 5.5 and run under DOSBox-X. It may use a controlled hook or stub for time, input, video capture, or allocation setup; the report must name that intervention. |
| **S** | Source-derived expectation | Expected behavior is transcribed or modeled from Pascal source, but that case was not executed in compiled Pascal. Includes model-only and source-inspection assertions. |
| **P** | Production-only regression | The production implementation is tested without a matching compiled-Pascal output for that case. Useful for regressions, not an oracle. |
| **DP** | Direct differential | A **D** capture is compared field-for-field or byte-for-byte with production output. |
| **I** | Integrated replay | A full engine session replays a world, normalized event tape, and deterministic time/RNG inputs from initial load through named checkpoints against the Pascal reference. A few direct routine calls or a short isolated loop do not count as an integrated replay. |

The current clean-suite record dated 2026-11-24 reports 83 direct Turbo Pascal 5.5 probe sessions (54 standard and 29 standalone), serializer/high-score/runtime suites, 42 configured Pascal-to-production differential invocations, and 16 additional direct `.HI` byte comparisons. This is strong focused evidence, but it does not satisfy the 10-fixture **I** corpus or prove whole-game compatibility.

## Current compatibility work

`[x]` means the stated scope has evidence; `[~]` means some cases pass but the subsystem remains open; `[ ]` means the required coverage is absent. A completed probe never implies that its entire subsystem is complete.

### Core architecture and platform boundary — [~]

- [x] Reuse a shared LibZoo engine and serializer for Android and Windows rather than maintaining duplicate gameplay engines.
- [x] Windows builds and engine/I/O smoke checks exercise the shared engine; Android host/runtime checks also exercise LibZoo.
- [~] Flood-fill traversal and `.HI` table initialization/load/reset/save/rank/insertion now use shared APIs in `third_party/libzoo/src/libzoo/zoo_editor.c` and `zoo_high_scores.c`, called by Android JNI/Java adapters. Placement/paste/stat mutation side effects remain in Android `native-lib.c`; the broader editor mutation API remains open. Four fill maps and 16 high-score byte cases compare directly against compiled Pascal.
- [ ] Put each in-scope semantic operation behind one platform-neutral implementation/API, leaving JNI, Win32, Android Java, filesystem, rendering, input, and sound-output adapters outside it. Do not fork or duplicate game rules to satisfy this item.

### World state, board lifecycle, and transitions — [~]

- [x] Direct checks cover world creation, title-only/full loading, board entry, selected spawn, dark-room message setup, neighbor-board transitions, blocked-entry rollback, same-row west/east entry, passage scan order, and destination start coordinates.
- [x] Direct checks cover stat addition/removal/movement, board preparation/placement, and selected game start/stop/reload lifecycle cases.
- [~] Board/world lifecycle has only focused routine and short-loop comparisons; saved-game resume, repeated board transitions, failure/retry sequences, and long-running integrated world sessions are not established.
- [ ] Cover every game-state transition, including load/title/play/death/restore/quit/board-change/error paths, with ordered side effects and state snapshots.

### Binary formats and file semantics — [~]

- [x] `.ZZT`/`.SAV` serialization has byte comparisons for representative world/save fixtures, including the Pascal `IsSave` distinction, title-only load, missing trailing records, short length/payload reads, raw Pascal string tails, Boolean/reserved bytes, CP437 bytes, board RLE, and unsigned `0xFFFF` record lengths.
- [x] Board RLE direct probes cover the 255-cell split, zero-count underflow/final-run overrun, and all 256 run-count byte values.
- [x] `.BRD` import/export has 16-record serializer checks plus direct cases for negative/unsigned length and truncated reads.
- [x] `.HI` table semantics use shared `zoo_high_scores.c`. Sixteen direct Pascal comparisons cover the 1,590-byte save record, eight load/reset inputs (missing, empty, lengths 1/52/53/1,589/1,590/1,597), and seven add outcomes (top, tie, last, cutoff, zero, negative, and edited name). Raw/NUL/CP437 bytes, signed scores, stale tails, trailing data, and accepted/rejected save behavior are included. A further 256-case insertion suite is source-model based, not 256 direct Pascal runs.
- [~] These are representative fixtures, not a complete field/boundary manifest for every operation and record type. The `.BRD` and `.HI` semantic cores are shared in LibZoo; Android retains stream acquisition, CP437 name entry, and editor reset/close policy.
- [ ] Verify every serialized field and each listed encoding/read edge below with direct Pascal outputs and the production implementation; define precisely which bytes are deterministic and which are untouched destination memory.

### Tick scheduling, clocks, timers, and RNG — [~]

- [x] `GAMESCHD.PAS` directly exercises five real game-loop cycles at tick speeds 1, 6, and 9, a scripted four-extra-PIT stall, and a start near the centisecond rollover; the production cycle timestamps/counters match.
- [x] `ZEROSPD.PAS` verifies five zero-speed cycles; deterministic scheduler tests cover fractional 60 Hz accumulation, catch-up, suspension/rebase, backward timestamps, and deadlines.
- [x] Direct sound-clock cases cover thresholds and minute rollover; sound timer probes cover duration underflow/wrap and note advancement.
- [x] Seeded direct element cases compare RNG state and call order for selected Lion, Tiger, Centipede, Spinning Gun, and other behaviors.
- [~] These checks do not cover the complete game-loop state machine, every caller/order of `Random`, pause/resume interactions, repeated clock stalls, or long integrated sessions.
- [ ] Finish the deterministic clock/RNG and timer acceptance matrix in blockers E2-03 and E2-07.

### Input queue and player behavior — [~]

- [x] Direct probes cover movement, torch activation, player state/damage, pause movement, monitor exit keys, Pascal arrow/numpad inputs, and selected keyboard buffer cases.
- [x] A direct `InputUpdate` check preserves CP437 bytes `0x82` and `0xFF`; the production bridge test checks extended-key prefix handling. `INPUTBUF.PAS` captures the unusual penultimate-byte behavior (`Copy(InputKeyBuffer, Length-1, 1)`) and repeated `R` burst behavior.
- [~] Current burst differential coverage is focused; all buffer lengths, overwrite/shift boundaries, multiple simultaneous actions, and engine effects across a multi-cycle input tape remain open. OS key repeat, touch conversion, and physical keyboard adapters are excluded.
- [ ] Complete the normalized-event queue and player-state matrix under E2-07 and the integrated scenarios under E2-10.

### Element definitions, ticks, touches, and interactions — [~]

- [x] All 54 Pascal/production element metadata rows are compared. The sole waived metadata difference is Player element 4: LibZoo uses an in-place draw callback while Pascal mutates `ElementDefs[4].Character`.
- [x] Direct probes cover the current matrix of Player, Object tick/touch, Bear, Lion, Shark, Tiger, Slime, Centipede, Ruffian, Star, Pusher/Duplicator, Spinning Gun, Bomb, Bullet/projectiles, Blink Wall, Scroll, Conveyor, Transporter, Message Timer, doors/keys, terrain, items, torch, Energizer, passages, damage, and selected draw frames.
- [x] Direct boundary cases preserve signed 16-bit counter wrapping, unchecked `Keys[0]`/Gems aliasing, asymmetric `#PUT` bounds, `#CHANGE` scan limits, and several element-specific call-order/color/under-tile quirks.
- [~] Per-element probes do not establish every callback branch, every element-pair interaction, every board-edge combination, or effects accumulated over normal stat iteration and complete game cycles.
- [ ] Use the Pascal element dispatch inventory to close callback and interaction coverage under E2-04; verify the Player draw-callback difference by resulting engine-cell snapshots.

### ZZT-OOP parser and interpreter — [~]

- [x] Direct parser probes include 775 word reads, 524 numeric reads, and 1,538 line reads; direct execution checks include 55 `#IF` outcomes, 42 named `#IF ANY` lookups, ten raw 21-byte flag slots, 24 `#GIVE`/`#TAKE` cases, 11 flow/movement snapshots, eight `#CHAR` cases, eight `#PUT` edge placements, and the full 1,497-cell `#CHANGE` map.
- [x] Direct cases cover `#SEND` self/cross-object locks and duplicate targets, `#BIND` selectors and alias/reuse behavior, plus commands such as `#CYCLE`, `#END`, `#GO`, and tested tile/flag/counter paths.
- [~] This is broad but not a complete command-dispatch/alias inventory or an integrated suite of interacting scripts. Error strings/cursor positions, malformed and unterminated source, numeric overflow, instruction budget/recursion/locking combinations, and side effects spanning multiple objects still need explicit coverage where the Pascal source defines them.
- [ ] Generate the complete command and alias inventory from `OOP.PAS`; satisfy E2-05 for every branch and combine scripts with real ticks under E2-10.

### Stats, heap, script data, and `#BIND` — [~]

- [x] Direct checks cover selected stat add/remove/move effects, `#BIND` selector outcomes, self/cross-object aliases, FIFO free-block reuse, adjacent free-block coalescing for measured requests, payload pointer deltas, and text-window allocation at the 50/51-byte reuse boundary.
- [x] Production regressions match measured DOSBox-X allocation traces, including `BINDREUS`, `BINDORDR`, `BINDSIZE`, `BINDTAIL`, `BINDWIN`, and shallow script-copy aliases.
- [~] Raw pointer addresses and reuse depend on compiler/runtime heap layout; only selected startup/allocation contexts are measured. Other allocation histories, exact exhaustion, malformed data lengths, and cross-board cleanup are open.
- [ ] Adopt the nondeterminism contract below and close all required alias/data/allocation boundary cases under E2-06. Do not require host pointer numeric equality.

### Engine-produced text cells and text windows — [~]

- [x] Direct captures compare selected gameplay draw callbacks, star/transporter glyph frames, message-row output, Scroll title/body rows, text-window labels/jumps, and restoration paths.
- [x] Logical character/attribute behavior is distinct from the Android 60×25 and Windows 80×25 raster front ends. The front-end font, scaling, DOS blink presentation, and screen/window layout are excluded here.
- [~] No complete normalized 80×25 cell/event transcript exists for all shared engine states and text-window edge cases. Video adapter details from Pascal are not themselves a portable engine requirement.
- [ ] Specify the virtual-cell capture boundary and satisfy the gameplay/text-window portions of E2-08. Compare CP437 byte and attribute writes, not rendered pixels.

### Sound event generation — [~]

- [x] Direct probes cover 72 frequency mappings, selected 112-byte and priority-tagged patterns, timer boundaries, note duration, `Sound; NoSound`, and drum sequence timing. Production parser/tick regressions pass for their stated cases.
- [~] The integrated prototype directly compares two priority-2 boulder-push `SoundQueue` calls and the resulting queue state/bytes. The timer is disabled in this fixture, so frequency dispatch, duration/timer progression, stop boundaries, and other producers remain uncovered; see [`2026-11-24-e2-10-sound-event-capture.md`](tools/compatibility/evidence/2026-11-24-e2-10-sound-event-capture.md).
- [~] Complete sound-event generation/order/priority coverage across gameplay has not been integrated. PC speaker, Android PCM, latency, sample rate, and audible output are adapters and are out of scope.
- [ ] Compare the logical queued event sequence and timing at every integrated checkpoint under E2-08/E2-10.

### Editor mutations, `.BRD`, and high scores — [~]

- [x] Flood fill has four direct Pascal maps; placement/paste checks cover patterns, protected player tile, plain paste, `MAX_STAT` refusal, shallow script aliases, CR/NUL payloads, and selected stat fields.
- [x] The shared `.HI` table API passes 16 direct compiled-Pascal byte comparisons: one save fixture, eight load/reset captures, and seven insertion outcomes. The Android CP437 input adapter has separate encoding checks. The 256 full-table insertion rows remain source-derived supplementary coverage.
- [~] Flood-fill traversal and high-score table semantics are shared; placement/paste still exercises Android JNI adapters in some probes. Editor.PAS board creation, properties, other stat/object mutations, undo/state, and save mutations lack direct paired probes. `.HI` file I/O and name-entry adapters are tested separately from shared table semantics.
- [ ] Close only semantic editor/high-score operations under E2-09. Do not add editor menus, prompts, cursor/input adapters, or visual layout to this engine checklist.

## Known quirks, discrepancies, and environment-dependent behavior

These are explicit constraints for the acceptance tests, not permission to normalize behavior.

| Case | Current evidence / issue | Compatibility requirement |
|---|---|---|
| Unread bytes after short Pascal reads | `SHORTRD.PAS` observed different unread-tail values (65, 0, 222) across fresh DOSBox-X runs. Pascal leaves unread destination memory untouched; production uses its already allocated destination. | Seed the destination with a known pattern, then compare that the unread suffix is unchanged. Compare bytes actually read and resulting control/state effects. Do not demand a particular allocator's uninitialized byte values. |
| Turbo Pascal heap addresses and freed-block reuse | `#BIND`, `GetMem`/`FreeMem`, stat and text-window allocations can reuse blocks according to allocation history; measured DOSBox-X sequences include 27/36-byte adjacent blocks and 50/51-byte boundaries. | Compare success/failure, block sizes, alias graph, bytes observed through aliases, and reuse order under the same explicitly seeded startup/allocation trace. Ignore absolute segment:offset or host pointers. A behavioral difference in aliases or data remains a blocker. |
| Raw fields left undefined by a fresh Pascal `WorldCreate` | The integrated `WorldSave` prototype emitted nonportable values in empty short-string payload tails, `WorldInfo.unkPad`, board name/message tails, uninitialized start-player coordinates, `TBoardInfo.unkPad`, and stat pointer/reserved fields. Across 961 `.SAV` bytes, the fresh-world comparison classifies 396 positions; 327 values differ and 69 happen to match. Reloading that exact Pascal-generated file and saving again matches all 949 preserved positions; only the 12 process-specific stat pointer bytes are classified. The probe sets `TWorldInfo.unk1` to zero because `WorldCreate` leaves it undefined and the production fixture starts from zeroed storage. | For a fresh-world save, compare every remaining serialized byte exactly and report each classified range. For a loaded file, compare all original raw bytes and saved fields exactly, except explicitly process-specific stat data-pointer bytes; do not reuse the fresh-world padding, string-tail, start-coordinate, or reserved-byte masks. Continue comparing string lengths/content, defined world/board/stat fields, scripts, and layout exactly. |
| Randomize seed | Original seed depends on DOS time; per-tick RNG call order is observable. | Inject identical BIOS time/initial `RandSeed`; require exact subsequent seed/state/results. Do not require two live hosts launched at different wall-clock times to choose identical random worlds. |
| PIT/clock scheduling | Real DOS loop cadence depends on timer interrupts, centisecond phase, and when the process polls; modern host scheduling is not deterministic. | Use a scripted logical clock/event tape and compare due-cycle decisions/counters. Do not require equal wall-clock latency or identical frame cadence from a host OS. |
| Sound hardware | DOS speaker and modern output devices differ in buffering and latency. | Compare logical frequency/duration/priority/order events and tick timing. Do not compare physical waveform, speaker response, or platform latency. |
| Player element metadata | The 54-row comparison intentionally waives `hasDrawProc` for element 4 because Pascal mutates the Player character and LibZoo uses a draw callback. | Keep the implementation difference only if resulting virtual cells, timing, and game state match Pascal on all Player animation cases. Otherwise fix it; do not broaden the waiver. |
| Truncated editor `.BRD` reads | `BRDIOPRB.PAS` directly captures I/O error 100, the partially updated length word, unchanged unread payload bytes, and a following `IOResult` of 0. `BRDXFER.PAS` directly executes the original nested `EditorTransferBoard` on a short payload; it reaches `BoardCreate` (`BoardLen=0`, max shots 255, zero stats, center Player at color 31), and the shared engine matches seven selected board/player fields for this trace. The earlier `SHORTHEAD` fixture was nine characters and therefore invalid for DOS 8.3 file access; corrected `SHORTHD` attempts stall or terminate with runtime error 203 while reading the freed BoardData. The final `BoardOpen` path is source-derived only. A separate production-only regression verifies the safe shared-engine reopen model from a seeded board, but it does not provide a direct Pascal differential. | Directly run the original editor import with seeded prior board state for both header and payload truncation; capture return/control path, resulting board, retained bytes, and relevant allocations. Compare those operations against one shared-engine import/error API under the same controlled history. Preserve unread bytes as untouched; model the inferred short-header board reopen with controlled semantics rather than freed-buffer access or C undefined behavior. OS stream-close details are adapter behavior. |
| Pascal integer/array quirks | Direct probes confirm signed 16-bit wrapping, the `Keys[0]` alias, board-edge asymmetries, row/column scan limits, and other cases listed above. | Preserve the observed operation order, width, bounds, and side effects. No “safer” arithmetic or bounds correction may silently change game behavior. |
| BIOS text adapter and glyph shape | Pascal requests CO80 text mode but does not load a font; original glyph pixels depended on the display adapter. | Engine tests compare emitted CP437 character bytes and attributes. Do not use raster/font resemblance to pass or fail engine compatibility. |

## Finite release blockers

Every blocker is mandatory. A blocker closes only with committed automated evidence, the named oracle/evidence class, and a clean rerun. “Works in a demo” or a production-only regression is not a substitute for a Pascal differential where direct execution is feasible.

### E2-01 — Shared engine boundary and source inventory — [~]

- [ ] Produce a source-to-subsystem inventory mapping every routine in `ZZT.PAS`, `GAMEVARS.PAS`, `GAME.PAS`, `ELEMENTS.PAS`, `OOP.PAS`, `EDITOR.PAS`, `INPUT.PAS`, `KEYS.PAS`, `SOUNDS.PAS`, `TXTWIND.PAS`, and `VIDEO.PAS` to a shared production routine and at least one test, or mark it explicitly as an adapter/out of engine scope with the reason. Video/input adapters may be excluded; mutations and decisions implemented in shared code may not.
- [x] Move `.BRD` record parsing and serialization into the shared LibZoo API (`zoo_editor_brd_*`); Android JNI calls it, and the serializer harness now compiles that same production source.
- [ ] Move the remaining editor board mutations through platform-neutral APIs; frontend code may only adapt files, controls, prompts, and display. There must be one implementation of each shared semantic operation, with no frontend-specific duplicate game rules.
- [ ] Keep one canonical test fixture/oracle path for both platform builds; no duplicated game rules or platform-specific engine fork.
- **Pass:** inventory has zero unclassified state-mutating routines; shared semantic APIs compile in each platform build that includes the engine and are called by any frontend exposing that operation; frontend-only adapters are identified and excluded.

### E2-02 — `.ZZT`, `.SAV`, `.BRD`, `.HI` byte and error compatibility — [~]

- [~] `BOARDENC.PAS` now captures the complete length-prefixed `.BRD` produced by the reconstructed Pascal `BoardClose`. A clean direct run imported and exported its 289-byte record through shared LibZoo with all record bytes identical (external file: 291 bytes; SHA-256 `e97bfa7b3c33355f54bd04e8574bf5eb3e4d14ad834918d021e590ad04a7dddd`). This is one valid `.BRD` source-output case only; the four-format manifest and field/boundary matrix remain open. See [`2026-11-25-e2-02-pascal-boardclose-binary.md`](tools/compatibility/evidence/2026-11-25-e2-02-pascal-boardclose-binary.md).
- [ ] Create a checked-in fixture manifest with, for each format, at least one real source-produced valid file and generated boundary fixtures for every serialized field/record. Compare full output bytes, not selected fields.
- [ ] Cover short-string lengths 0, 1, 49, 50, 51, and raw length bytes 127/255; embedded NUL and CP437 bytes 0x80, 0x82, 0xFF; Boolean bytes 0, 1, 2, 255; reserved and stale tails; signed integer values `-32768`, `-1`, `0`, `1`, `32767`; board/stat counts 0, 1, maximum-1, maximum; and RLE counts 0, 1, 254, 255 plus a 256-cell run split.
- [ ] Cover empty/missing, valid, extra trailing data, title-only, absent trailing board records, partial length word, partial payload, stored `0xFFFF`, zero-count RLE, and truncation at every length/payload boundary for `.ZZT`/`.SAV`/`.BRD`/`.HI` where applicable.
- [ ] For every case, run the compiled Pascal operation and production operation from equivalent seeded state; compare bytes, returned success/error class, board/world reset/preservation, and untouched buffer suffix according to the nondeterminism table.
- **Pass:** all listed cases have **D** captures and **DP** comparisons; zero unexplained byte/state/error mismatches. No case relies only on the Java/C model. Preserve the documented `.SAV` `IsSave` byte distinction and the DOS-supported semantics of each operation.

### E2-03 — World/game lifecycle and logical time — [~]

- [ ] Compare direct Pascal and production transitions for initial world load, title-to-play, board entry/change, passages, death, restore, pause/resume, save/return, quit/abort, malformed-load failure, and retry. Include both successful and refused/blocked transitions.
- [ ] Run a deterministic schedule matrix with `TickSpeed` 0, 1, 6, and 9; 5 cycles per speed; start phases 0 and 5,980 centiseconds; normal clock; a four-PIT stall after poll two; pause/resume across a pending tick; and minute rollover. Reuse existing passing cases and add only currently missing cases.
- [ ] Compare after each logical cycle: world/board identity, tick counters and phase, board timer, message timer, player/stat state, input consumed, queued sound, and cell writes. Use scripted time; wall-clock equality is not a criterion.
- **Pass:** every transition/schedule row has direct **D**/**DP** evidence; exact state/order matches at each checkpoint; no duplicate or skipped logical tick against the scripted clock.

### E2-04 — Element callbacks and interaction rules — [~]

- [ ] Build an inventory of all 54 element definitions and all Pascal `Tick`, `Touch`, and `Draw` callbacks. For each non-default callback, test every dispatch branch at least once directly; for default/no-op callbacks, prove the source and production both take that path.
- [ ] For each distinct touch handler, test a triggered and a non-triggered case; include player/enemy/projectile/solid/water/door/key/passages, protected Energizer, blocked and successful push/shoot, board edge, under-tile restore, and stat removal/reordering effects where applicable.
- [ ] Preserve and compare RNG call order, signed wrap, sound bytes/priority, tile element/color, stat order/fields, under-tile, deltas, messages/flags, and draw writes for each callback case.
- [ ] Require combined interaction coverage from E2-10; isolated callback coverage alone cannot close this blocker.
- **Pass:** callback inventory has 100% accounted coverage, each non-default branch has direct **D**/**DP** evidence, all currently listed element quirks remain pinned, and integrated interactions have zero mismatches.

### E2-05 — ZZT-OOP parse, dispatch, and execution — [~]

- [ ] Enumerate every command token, alias, conditional operator, direction/tile token, and parser error path from `OOP.PAS`; tests must fail if source dispatch inventory changes without new oracle rows.
- [ ] Directly compare each command/alias's consumed bytes/cursor, `DataPos`, stats, tiles, flags, counters, messages, RNG/sounds, and control-flow result. Include empty/long line, CR/LF/NUL/high-byte, case, numeric boundaries/overflow, malformed values, same-line trailing text, missing/duplicate labels, lock states, and bounded termination/recursion cases actually defined by the Pascal program.
- [ ] Retain existing direct parser counts and edge tests; add missing command branches, not redundant copies of passing probes.
- [ ] Run scripts with multiple interacting objects and real cycles in E2-10.
- **Pass:** 100% of the source command/alias dispatch inventory has direct Pascal coverage and a production comparison; all source-defined error/edge paths are represented; no unclassified command remains.

### E2-06 — Stat operations, heap, text-window allocations, and `#BIND` — [~]

- [ ] Compare `AddStat`, `RemoveStat`, `MoveStat`, board preparation, board cleanup, and all `#BIND` selectors under controlled source-faithful startup allocation and free histories.
- [ ] Include allocation request sizes 0, 1, 26–70, 255, and every observed script/text-window allocation boundary; exercise single and adjacent frees, multiple free blocks, first-fit/reuse order, repeated binds, self/cross-object aliases, source/recipient removal, and board change/unload.
- [ ] Compare allocation result, stat count/order/fields, `DataLen`/`DataPos`, alias graph, and bytes observed through each alias. Ignore numeric pointer/segment values; compare only where the same logical block aliases and what it exposes.
- [ ] Repeat each allocation trace in a fresh Pascal run; if the Pascal allocator result itself varies, record the controlled launch/setup and use the behavior-preserving envelope specified in the nondeterminism table.
- **Pass:** all existing `BIND*` cases remain green; all required histories have direct **D**/**DP** tests; there are no unexplained alias, data, stat-order, or allocation-success differences.

### E2-07 — Input queue, player control state, RNG, and tick callers — [~]

- [ ] For the engine's normalized input queue, directly compare empty, one-byte, every buffer-length boundary 0–255, wrap/shift, repeated identical keys, and mixed-byte sequences including extended-key and CP437 bytes. Assert consumed-byte order and exact residual buffer.
- [ ] Exercise all input-to-engine actions that are part of the shared core, including fire/shoot tap, direction, pause, menu decision, and reset after death; exclude keyboard layout, Android touch, Win32 messages, and host key-repeat timing.
- [ ] Inventory every Pascal `Random` call site. Compare the PRNG primitive over a fixed documented seed vector including zero, wrap/high-bit and existing gameplay seeds; compare every call site's `Random(n)` range, call order, resulting RNG state, and gameplay decision against direct Pascal probes with identical initial seed.
- [ ] Exercise existing tick speed/clock cases plus any unresolved timer callers surfaced by the call-site inventory. Use injected BIOS time and logical ticks.
- **Pass:** 100% input mutation paths and RNG call sites accounted, exact queue/residual bytes and seeded random decisions match, and no OS event timing is used as a substitute for logical semantics.

### E2-08 — Engine text-cell effects, text windows, and sound events — [~]

- [ ] Add a platform-neutral capture of engine-emitted virtual cells/events at Pascal `VideoWriteText`/game draw boundaries. Do not capture window-client pixels or a platform raster.
- [ ] Cover the 80×25 logical cell coordinate/attribute range wherever shared engine code writes it; compare CP437 character byte, foreground/background/blink bits, write order, and restored underlying cells for gameplay draw, `DrawPlayerSurroundings`, message rows, Scroll/text-window title/body, label jump, append, and close/restore.
- [ ] Directly compare logical sound events (frequency, duration, priority, queue order, timer/stop boundary) for every source sound producer exercised by gameplay; sound device output remains excluded.
- [ ] Check the element-4 Player callback difference against resulting logical cell writes; the metadata waiver alone is insufficient.
- **Pass:** every source-level engine cell/sound producer is inventoried and has direct coverage or is proved presentation-only; all direct captures match byte/event-for-byte with zero unexplained differences.

### E2-09 — Editor mutation and high-score semantics — [~]

- [ ] Inventory all state/file operations in `EDITOR.PAS`, excluding menu drawing, prompts, physical key handling, and editor cursor presentation. Cover board create/append/select/delete, board properties, tile/place/fill, object/stat add/remove, copy/paste/script aliasing, `.BRD` import/export, and save/dirty-state effects where present in the reconstructed source.
- [ ] Compare each operation's board/world state and bytes against direct Pascal. Retain the existing four fill maps and placement/copy probes; add only uncovered mutation branches and malformed/limit cases.
- [x] Direct `.HI` checks pass for initialization, load/reset, exact 30-record save, rank 1/rank 30/cutoff, duplicate ties, zero/negative/boundary scores, edited names, malformed/truncated/trailing tables, CP437/NUL/stale tails, and accepted/rejected save behavior. Sixteen byte-for-byte compiled-Pascal comparisons use the shared LibZoo implementation; 256 source-model rows remain supplemental.
- [~] The shared `.HI` implementation is exercised directly, but other editor operations still use or test Android adapter helpers and need to move behind the platform-neutral API.
- **Pass:** every in-scope source mutation and high-score data branch is direct **D**/**DP**, both `.BRD` and `.HI` use the shared semantic module, and UI-only routines are listed as excluded rather than accidentally untested core logic.

### E2-10 — Integrated gameplay replay corpus — [ ]

- [~] The deterministic `INTEGPRB` replay runs the original paused `GamePlayLoop`, buffered right-key handling, two boulder pushes, and two interacting objects whose scripts execute `#GIVE GEMS 1` / `#IDLE` and `#GIVE AMMO 1` / `#IDLE` over six zero-speed cycles. Dynamic snapshots now include loaded state, loop start, pause-to-play, six cycles, post-save and post-reload transitions, and final state: `L0`, `S0`, `T1`, `C1`–`C6`, `T2`, `T3`, `F0`. The clean comparison matched 41,690 state fields across 12 checkpoints, two logical `SoundQueue` events (20 event fields), and nine ordered playfield cell writes (36 event fields). A 961-byte fresh-world `.SAV` matched at all 565 stable positions; 396 fresh-world undefined/pointer positions are classified (327 differ; 69 happen to match). Both implementations then load the exact same Pascal-generated `.SAV`: compiled Pascal reports `WorldLoad=true`, production `zoo_world_load` returns 0, and the second 961-byte save matches all 949 positions except the 12 process-specific stat data-pointer bytes. The probe explicitly seeds otherwise-undefined `TWorldInfo.unk1` to zero and runs the compiled probe in a fresh DOSBox-X session after a separate compile session to control the compiler's inherited DOS allocation history; neither intervention changes gameplay semantics. `WorldSave` itself has no return value, so failure-path outcome coverage remains open. This is still one prototype and does not count as any of the ten required fixtures; broader scenarios and transitions remain outstanding. The clean 2026-11-25 run passed 84 direct sessions and 43/43 configured comparisons. See [`2026-11-25-e2-10-state-transition-capture.md`](tools/compatibility/evidence/2026-11-25-e2-10-state-transition-capture.md), [`2026-11-25-e2-10-save-byte-capture.md`](tools/compatibility/evidence/2026-11-25-e2-10-save-byte-capture.md), and the new save/load round-trip evidence note. This prototype does not replace any of the ten required fixtures.
- [ ] Create a finite, versioned set of at least **10** replay fixtures using original or purpose-built worlds and fixed normalized input/time/RNG tapes. Required scenario coverage: (1) DEMO start and ordinary traversal; (2) walls, pushers, duplicators, and collision ordering; (3) combat, bullets, keys/doors, and pickups; (4) object stat order plus multiple interacting OOP scripts; (5) board edges, neighbor transitions, and passages; (6) dark room, torch, Energizer, timers, and death; (7) pause, zero/positive-speed scheduling, and delayed input; (8) save, restore, world reload, and board change; (9) editor mutation, `.BRD` round-trip, and play of the edited board; (10) malformed-file recovery and high-score insertion path.
- [ ] For each fixture, run the source-derived DOS program compiled with Turbo Pascal 5.5 and the platform-neutral production engine from the same initial files/state. Record named checkpoint snapshots at initial load, every logical game cycle, each board/state transition, and final save/exit. Extend the current snapshot schema to cover the remaining state and event fields listed below. Hook only nondeterministic environment sources (clock, keyboard tape, filesystem location, video capture, and seeded allocator setup); do not replace gameplay routines in the oracle.
- [ ] Compare world and board fields, all 60×25 playable cells plus every source-defined border/sentinel cell, stats and order, script bytes/cursors/aliases, flags/counters/timers/RNG state, emitted logical cell writes, sound event queue, return/error result, and generated file bytes. Apply only the explicit nondeterminism rules above.
- **Pass:** all 10 fixtures finish deterministically under their tapes with zero unexplained per-checkpoint differences. Any mismatch is triaged as a compatibility defect or an explicitly justified environment-dependent case; it cannot be waived as “visual only” when engine state/events differ.

### E2-11 — Clean release verification and evidence report — [ ]

- [ ] After E2-01 through E2-10 are closed, run the complete clean engine suite from a fresh verification staging directory: `tools/pascal-compat/verify.ps1` with the recorded Turbo Pascal 5.5 toolchain and DOSBox-X paths, then standalone `tools/serializer-compat/verify.ps1`, `tools/highscore-compat/verify.ps1` with direct Pascal fixture inputs, and `tools/runtime-compat/verify.ps1`.
- [ ] Include all new **D**, **DP**, and **I** replay runners in the clean suite; record tool versions/paths, commands, exit codes, fixture revision, and summary counts. Run twice from clean staging for any heap-sensitive or allocator-dependent test and explain any variance.
- [ ] Review the final known-difference table against results and retain only environment-dependent limits or explicitly scoped frontend exclusions. A real semantic mismatch is a release blocker, not a limitation note.
- [ ] Confirm Android/Windows front ends still consume the shared engine only as a non-regression check; device tests, frontend build/release operations, rendering, adapters, and packaging do not gate Engine 2.0.
- **Pass:** every mandatory blocker is checked, the complete clean suite is green, the evidence report identifies all **D/S/P/DP/I** coverage, and remaining limitations are explicitly non-blocking under this document.

## Optional future investigations

These are not Engine 2.0 blockers unless evidence shows an actual compatibility defect:

- Broader randomized/fuzzed file corpora beyond the field and malformed-boundary matrix in E2-02.
- Additional DOSBox/PCem BIOS, DOS release, or Turbo Pascal heap-layout surveys; numeric segment/address matching across different hosts is not required.
- New world corpus expansion beyond the 10 fixed integrated replay fixtures.
- Performance profiling, cleanup/refactoring, diagnostics, or API redesign that does not change behavior.
- Any modern UI, larger map, new feature, different save system, graphics/audio enhancement, or convenience behavior. These change the product surface and are outside this compatibility milestone.

## Completion record

Fill this only after E2-01 through E2-11 pass. Do not mark the engine complete on the basis of this checklist's creation or a clean focused suite alone.

- [ ] All finite Engine 2.0 blockers E2-01–E2-11 are closed with linked evidence.
- [ ] Complete clean verification suite passed on: ____________________
- [ ] Integrated fixture revision / count: ____________________
- [ ] Remaining non-blocking environment-dependent limitations: ____________________
- [ ] Engine 2.0 completion date and reviewer: ____________________

Until this section is filled with evidence, the milestone remains **in progress**.
