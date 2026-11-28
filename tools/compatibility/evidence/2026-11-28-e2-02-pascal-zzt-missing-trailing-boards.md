# E2-02: full `.ZZT` load with absent trailing board records

**Date:** 2026-11-28 (America/Chicago)  
**Implementation commit:** local `8d19daab7532f629e058ac95f723034c947dcc40` (tree `65b666e72e559115628494e046c41816ac5efab8`)  
**Published commit:** `d76e4b4581e9f2ba85e17b2909b791f1c9c0f0e1`  
**Evidence:** direct compiled Pascal (**D**) plus differential comparison of deterministic world fields and the complete on-disk board record (**DP**).  
**Inventory:** existing E2-02-AC001 and AC002 only. The inventory remains frozen at 54 cases; no acceptance ID or integrated-fixture credit was added. E2-02 remains partial.

## Probe and result

`FULLLOAD.PAS` initializes the reconstructed Pascal world, sets `World.BoardData[1..MAX_BOARD]` to `nil`, and calls the original `WorldLoad('FULLFILE', '.ZZT', false)`. The 1,649-byte `FULLFILE.ZZT` declares 15 boards but contains only a valid board 0 record. After recording load state, the Pascal probe calls the original `BoardClose` and writes the complete `World.BoardData[0]` record to `FULLBORD.BIN`. The production harness loads the same fixture through shared `zoo_world_load`, records the same state fields, then calls shared `zoo_world_save`; the comparator extracts board 0 from that saved `.ZZT` and compares its full file-format record with Pascal.

Pascal output (`FULLLOAD.OUT`, 55 bytes, SHA-256 `58f59910cf5505eead48d0ded298ac51e157279a4979b264e40097523a7b6ce1`):

```text
full-load 1 15 0 0 1 missing-len 0 0 data-present 1 0
```

The seven deterministic values match between Pascal and production: load success, declared board count, current board, `IsSave`, board-0 data presence, and the lengths of missing boards 1 and 15. The fixture is 1,649 bytes with SHA-256 `82a6352e96c1cbe5f09f1f23bdc5696b9e63445191fb38159240b57e6ae22cdf`.

The Pascal `FULLBORD.BIN` output is 1,137 bytes including its two-byte length prefix (SHA-256 `fc62b842453fb8b84233923f2a7338aaed694b634d3454570f0149c2926a50c4`). The record length is 1,135 bytes; its payload SHA-256 is `a3d2047bfb634fa41945e9469166bb6ef18fdd8a47e99a894d681146ac3082a4`. The production `zoo_world_save` board-0 record matched all 1,135 bytes; the first differing byte check found no difference.

An initial harness extension compared Pascal's file-format `BoardClose` record directly with LibZoo's private `world.board_data[0]` buffer after `zoo_board_close`. That private buffer is an internal representation (1,159 bytes here) with additional runtime metadata, so its raw bytes and length are not the same serialization boundary. The harness now compares the production `WorldSave` file output instead. This was a test-boundary correction; no engine code or Pascal output was changed to accommodate the comparison.

The two trailing pointer-presence observations are D-only and differ: under this controlled Turbo Pascal 5.5/DOSBox-X allocation history, `GetMem(0)` returned a non-`nil` pointer for board 1 and `nil` for board 15 (`1/0`); production stores `NULL` for both zero-length board records (`0/0`). Three fresh DOSBox-X probe sessions with the same setup reproduced the Pascal `1/0` pattern. Numeric pointer values were not compared. The zero-size allocation result is recorded separately from the seven matched fields, so the differential is not reported as a full pointer-state match.

## Compatibility boundary

Pascal `BoardOpen` dereferences the selected board buffer without checking its recorded length. Opening one of these absent boards could therefore expose bytes outside the zero-length allocation. That downstream effect was not invoked or compared in this probe. The production reader bounds reads by the serialized record length and returns zero bytes when the record is empty. Do not infer parity from this load-only differential, and do not reproduce out-of-bounds memory reads in production without additional evidence that establishes a stable, observable behavior under a controlled allocation history.

The implementation change is confined to the probe and comparator; no engine or Pascal gameplay semantics were changed and no oracle outputs were edited to fit production.

## Clean verification

The complete clean Turbo Pascal 5.5/DOSBox-X 2026.10.01 runner exited 0 on local source commit `8d19daab7532f629e058ac95f723034c947dcc40`, tree `65b666e72e559115628494e046c41816ac5efab8`, published as `d76e4b4581e9f2ba85e17b2909b791f1c9c0f0e1`. It passed 150 direct Pascal sessions (55 standard, 95 standalone), 75/75 configured Pascal-to-production comparisons, and F01–F10 (238 checkpoints, 824,821 state fields, 169 logical cycles). The full-load missing-trailing-board probe matched seven deterministic fields and all 1,135 serialized current-board bytes. No unexplained comparable state or byte mismatch was reported. The zero-length allocation observation and unverified absent-board open remain outstanding. The board-record check is included in the existing serializer comparator invocation; the configured differential count remains 75.

Command: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain 'build/pascal-compat-run/Z' -DosboxExecutable '<external DOSBox-X 2026.10.01 executable>'; exit 0`. Turbo Pascal 5.5 and DOSBox-X are external verification dependencies and are not redistributed.
