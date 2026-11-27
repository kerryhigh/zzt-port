# Direct Pascal `.BRD` RLE boundary compatibility

- Evidence class: **D+DP**.
- Acceptance case: `E2-02-AC002` (the RLE submatrix is complete; AC002 remains partial).
- Direct decoder probe: `tools/pascal-compat/BOARDRLE.PAS` invokes original `BoardOpen` for every run-count byte from 0 through 255. It writes all 1,500 decoded tile cells and the following `MaxShots` byte for each run.
- Production differential: `tools/serializer-compat/compare-board-rle.js` compares every 3,001-byte Pascal snapshot against the production shared reader. All 256 cases and **768,256/768,256 snapshot bytes** matched, including count zero consuming 256 cells through byte underflow and the following board-info byte remaining aligned.
- Direct writer boundary: `tools/pascal-compat/BOARDENC.PAS` loads the source world, fills the 60×25 map with one identical tile, and calls original `BoardClose`. The Pascal output records run lengths `255, 255, 255, 255, 255, 225`, covering the full 1,500-cell map and the 255-cell maximum run split.
- Production differential: `tools/pascal-compat/compare-pascal-boardenc.js` validates the Pascal record length, compares the full 289-byte `.BRD` record payload after shared-engine import/export, and compares the complete **25,928-byte** Pascal `.ZZT` output with shared `BoardClose` plus `WorldSave`; both comparisons pass byte-for-byte.
- The clean Turbo Pascal 5.5/DOSBox-X 2026.10.01 compatibility run on 2026-11-27 exited 0. The verifier runs both RLE comparators in `tools/pascal-compat/verify.ps1`; no Pascal gameplay or serializer semantics are patched for these probes.
- The RLE values and writer split listed by AC002 are verified. Remaining AC002 work is the other format/field boundary matrix and applicable malformed/truncated input and untouched-buffer comparisons. No acceptance ID or integrated fixture was added.
