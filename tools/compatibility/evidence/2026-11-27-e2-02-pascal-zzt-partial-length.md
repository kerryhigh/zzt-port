# E2-02: partial `.ZZT` board-length word

**Date:** 2026-11-27  
**Implementation commit:** `b617c48eb25969a03add95f887ae0aca1a115b76` (tree `e0931ff3f93146b431c4a67f7f8258874e3d3c71`)  
**Mirrored local commit:** `5ce54c2303a41e035d1502fc46db1f250bb7620b`  
**Evidence:** direct compiled Pascal (**D**) plus shared-engine differential (**DP**).  
**Inventory:** existing E2-02-AC001 through AC005 only; no acceptance ID or fixture-corpus credit was added. E2-02 remains partial.

## Case and oracle path

`SHORTLEN.PAS` initializes the original world, sets board 1's prior length to `0x0100`, fills its existing 256-byte allocation with `0x5A`, and calls the original full `WorldLoad`. The generated `PARTLEN.ZZT` ends after the low byte `0x10` of board 1's length word. Turbo Pascal's one-byte `BlockRead` changes only the low byte, retaining the prior high byte, so the load succeeds with length `0x0110`.

The production harness seeds the same prior board state and calls the shared `zoo_world_load` path on the exact Pascal-generated fixture. Its snapshot is compared with the direct Pascal output for load result, board count, current board, `IsSave`, board length, and board-data presence. No reconstructed loader, serializer, or production gameplay semantics are modified.

## Recorded result

| Artifact | Size | SHA-256 |
|---|---:|---|
| `PARTLEN.ZZT` | 1,650 bytes | `76ec912c33de4d742133a6f6016270f106295ad86033c9235f029dbdfde6ac5f` |
| `SHORTLEN.OUT` | 26 bytes | `039bb3873ae5a7f73e173cafc47e79ae11961fc21f6b290c9de88a8add289f10` |

Pascal and production matched all six values: `1 15 0 0 272 1`. These represent successful load, `BoardCount=15`, `CurrentBoard=0`, `IsSave=false`, `BoardLen[1]=272 (0x0110)`, and a present board-data allocation. The full run reported no unexplained mismatch.

## Clean verification

The complete clean Turbo Pascal 5.5/DOSBox-X run exited 0 and printed 150 direct Pascal sessions (55 standard, 95 standalone), with 75/75 configured Pascal-to-production comparison invocations. The partial-length comparison is an additional case inside the existing serializer differential invocation, so the invocation count remains 75. Integrated fixtures F01–F10 passed at 238 checkpoints, 824,821 state fields, and 169 logical cycles. The integrated negative control also passed.

The clean invocation was `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain 'build/pascal-compat-run/Z' -DosboxExecutable '<external DOSBox-X 2026.10.01 executable>'`; the absolute external executable path is omitted from this public record.

This verifies one controlled `.ZZT` partial-length case. Other field boundaries, malformed/truncated format cases, allocator histories, and untouched-buffer comparisons remain open under E2-02.
