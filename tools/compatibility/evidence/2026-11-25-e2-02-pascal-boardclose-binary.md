# E2-02 Pascal BoardClose binary differential

**Date:** 2026-11-25  
**Evidence:** DP (compiled reconstructed Pascal and shared production LibZoo)  
**Status:** Partial evidence for E2-02 AC001/AC004; neither case nor E2-02 is closed.

## Fixture and comparison

`tools/pascal-compat/BOARDENC.PAS` loads the checked-in DEMO world, fills the active 60×25 board with empty tiles using color `0x70`, and calls the original reconstructed-Pascal `BoardClose`. An output-only probe addition writes the resulting `.BRD` record with the DOS two-byte length prefix to the staging file `BOARDENC.BRD`.

The clean Turbo Pascal 5.5 / DOSBox-X runner passed the resulting file to `tools/pascal-compat/compare-pascal-boardenc.js`. That comparator calls `run_editor_brd_record_roundtrip` in the WebAssembly harness, which invokes shared LibZoo `zoo_editor_brd_import_record`, parse, and serialize routines. It then compares every record byte, including name, RLE data, board information, and stat data.

| Output | Size | Result |
|---|---:|---|
| `BOARDENC.BRD` | 291 bytes including length prefix | SHA-256 `e97bfa7b3c33355f54bd04e8574bf5eb3e4d14ad834918d021e590ad04a7dddd` |
| Length-prefixed record payload | 289 bytes | All bytes matched after shared LibZoo import/export |

The fixture is generated in the ignored verification staging directory; no generated binary is committed. The reconstructed reference gameplay source is untouched. Instrumentation is confined to the test probe's output capture.

## Verification

The full `tools/pascal-compat/verify.ps1` runner completed with exit code 0 on 2026-11-25 using Turbo Pascal 5.5, DOSBox-X 2026.10.01 (SHA-256 `072c1944635ccc545a816e128625e3405feab559021e4ef60b2d512f97f754c0`), Node.js 22.19.0, and Python 3.13.7. It passed 84/84 direct Pascal sessions and 44/44 configured Pascal-to-production comparisons. The added board comparison is one of those 44. The complete run also passed the existing serializer, high-score, runtime, and integrated replay gates; the integrated replay remains a prototype with 0/10 required fixtures.

## Limits

This evidence covers one valid source-produced `.BRD` record and its full-byte shared import/export round-trip. It does not provide the checked-in four-format fixture manifest, generated boundary fixture coverage for every field, all truncated-input/error outcomes, or direct comparisons for `.ZZT`, `.SAV`, and `.HI` across their full matrices. E2-02 AC001 and AC004 remain partial; the release blocker remains open.
