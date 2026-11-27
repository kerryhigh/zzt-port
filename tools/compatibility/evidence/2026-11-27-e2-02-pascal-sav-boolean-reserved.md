# E2-02 AC002: direct Pascal .SAV Boolean and reserved bytes

**Result:** four direct compiled-Pascal captures and production differentials passed; E2-02 AC002 remains partial.

The existing WORLDSAV.PAS probe loads the source-produced DEMO.ZZT and calls the original WorldSave routine. For BLS0.SAV through BLS3.SAV, it writes the raw byte values 0, 1, 2, and 255 into all seven World.Info.Keys Boolean fields and World.Info.IsSave using one-byte Move setup, and seeds all 14 World.Info.unkPad bytes with the repeated pattern 00 01 02 FF. The Pascal gameplay and serialization routines are unmodified.

compare-pascal-world-booleans.js checks the seven key bytes at offsets 8–14, IsSave at offset 264, the reserved tail at offsets 265–278, then passes each complete 26,465-byte file through the production world read/round-trip path. Each complete output matched byte-for-byte.

| File | Raw value | Bytes compared | SHA-256 |
|---|---:|---:|---|
| BLS0.SAV | 0 | 26,465 / 26,465 | 2A8B3787E0C3B7C9CAC3BE8E6F5A0345E39DF52297703F574576A918E549E346 |
| BLS1.SAV | 1 | 26,465 / 26,465 | 1C42AF092633EEEBB2F90AC15D4C0BB4F707A29341837709541A2DAC5053D7CA |
| BLS2.SAV | 2 | 26,465 / 26,465 | 484D41D8EA8972F1F94E2273A9C8BA1D10BF2148E38E1136704BCD293CF1FDF7 |
| BLS3.SAV | 255 | 26,465 / 26,465 | CECE6A30613AD0C19BC84D8174B59BF13BD6503EA522D9A66A0E6EB1FF9F45BA |

**Full clean confirmation:** root source revision b4d6b8c9b3c7769d7b6be52ce61b6393a0b1eac5; Turbo Pascal 5.5 with DOSBox-X 2026.10.01; tools/pascal-compat/verify.ps1 exited 0 with 148 direct Pascal compile/run sessions and 70/70 configured Pascal-to-production comparisons. The .SAV comparator ran once in that clean suite. Its outputs are generated in ignored staging and are not committed as binary fixtures.

**Evidence class:** D+DP. This covers these .SAV Boolean and reserved bytes only. Boolean/reserved and stale-tail variants in other formats, DataLen, count edges, RLE boundaries, and applicable malformed/truncated inputs remain part of the frozen AC002 matrix.
