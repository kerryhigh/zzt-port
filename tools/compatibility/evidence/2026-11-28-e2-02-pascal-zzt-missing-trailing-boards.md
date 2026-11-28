# E2-02: full `.ZZT` load with absent trailing board records

**Date:** 2026-11-28 (America/Chicago)  
**Implementation commit:** local `f99756b6efaee10af92011593c87b158ce9a0b94` (tree `a748eaaf0b207e643d277a7f9ff2ea27db09d019`)  
**Published commit:** `6c3b0287d7c29f4076cea7187549945dee749e87`  
**Evidence:** direct compiled Pascal (**D**) plus differential comparison of deterministic fields (**DP**).  
**Inventory:** existing E2-02-AC001 and AC002 only. The inventory remains frozen at 54 cases; no acceptance ID or integrated-fixture credit was added. E2-02 remains partial.

## Probe and result

`FULLLOAD.PAS` initializes the reconstructed Pascal world, sets `World.BoardData[1..MAX_BOARD]` to `nil`, and calls the original `WorldLoad('FULLFILE', '.ZZT', false)`. The 1,649-byte `FULLFILE.ZZT` declares 15 boards but contains only a valid board 0 record. The production harness passes the same fixture to the shared `zoo_world_load` routine and captures world-load state plus board-buffer presence.

Pascal output (`FULLLOAD.OUT`, 55 bytes, SHA-256 `58f59910cf5505eead48d0ded298ac51e157279a4979b264e40097523a7b6ce1`):

```text
full-load 1 15 0 0 1 missing-len 0 0 data-present 1 0
```

The seven deterministic values match between Pascal and production: load success, declared board count, current board, `IsSave`, board-0 data presence, and the lengths of missing boards 1 and 15. The fixture is 1,649 bytes with SHA-256 `82a6352e96c1cbe5f09f1f23bdc5696b9e63445191fb38159240b57e6ae22cdf`.

The two trailing pointer-presence observations are D-only and differ: under this controlled Turbo Pascal 5.5/DOSBox-X allocation history, `GetMem(0)` returned a non-`nil` pointer for board 1 and `nil` for board 15 (`1/0`); production stores `NULL` for both zero-length board records (`0/0`). Three fresh DOSBox-X probe sessions with the same setup reproduced the Pascal `1/0` pattern. Numeric pointer values were not compared. The zero-size allocation result is recorded separately from the seven matched fields, so the differential is not reported as a full pointer-state match.

## Compatibility boundary

Pascal `BoardOpen` dereferences the selected board buffer without checking its recorded length. Opening one of these absent boards could therefore expose bytes outside the zero-length allocation. That downstream effect was not invoked or compared in this probe. The production reader bounds reads by the serialized record length and returns zero bytes when the record is empty. Do not infer parity from this load-only differential, and do not reproduce out-of-bounds memory reads in production without additional evidence that establishes a stable, observable behavior under a controlled allocation history.

The implementation change is confined to the probe and comparator; no engine or Pascal gameplay semantics were changed and no oracle outputs were edited to fit production.

## Clean verification

The complete clean Turbo Pascal 5.5/DOSBox-X 2026.10.01 runner exited 0 on local source commit `f99756b6efaee10af92011593c87b158ce9a0b94`, tree `a748eaaf0b207e643d277a7f9ff2ea27db09d019`, published as `6c3b0287d7c29f4076cea7187549945dee749e87`. It passed 150 direct Pascal sessions (55 standard, 95 standalone), 75/75 configured Pascal-to-production comparisons, and F01–F10 (238 checkpoints, 824,821 state fields, 169 logical cycles). No unexplained comparable state or byte mismatch was reported. The allocation observation above remains a known, unverified downstream edge case. The missing-trailing comparison was added to the existing serializer comparator invocation; the configured differential count remains 75.

Command: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain 'build/pascal-compat-run/Z' -DosboxExecutable '<external DOSBox-X 2026.10.01 executable>'; exit 0`. Turbo Pascal 5.5 and DOSBox-X are external verification dependencies and are not redistributed.
