# Direct Pascal WorldSave board-count boundary differential

- Evidence class: **D+DP**.
- Reference operation: reconstructed Pascal `WorldSave` in `WORLDSAV.PAS`, compiled with Turbo Pascal 5.5 and run in the clean DOSBox-X 2026.10.01 verification session.
- Production operation: shared LibZoo `zoo_io_world_read`, `zoo_board_open`, `zoo_board_close`, and `zoo_io_world_write` through `world_roundtrip.wasm`.
- Engine source revision: `b4d6b8c9b3c7769d7b6be52ce61b6393a0b1eac5` (engine code unchanged for this probe).
- Source constant: `MAX_BOARD=100` in `GAMEVARS.PAS`; the serialized count is the last board ID, so count 100 contains 101 board records.
- Acceptance case: `E2-02-AC002` (partial; no new acceptance case or fixture-corpus completion credit).

The existing direct `WORLDSAV.PAS` session copies board 0's serialized record into independently allocated Pascal buffers for board IDs 1 through 100. This keeps each slot valid and independent because the original `WorldSave` closes the active board by freeing and replacing that board's buffer. It then writes counts 0, 1, `MAX_BOARD-1` (99), and `MAX_BOARD` (100) as both `.SAV` (`IsSave=1`) and `.ZZT` (`IsSave=0`). No reference gameplay or serializer semantics are modified.

The production comparator verifies the version marker, signed board-count word, `IsSave` byte, every length-prefixed board record, absence of extra trailing bytes, and a complete production load/open/close/save round-trip for each file. All 8 full files match byte-for-byte:

| File | BoardCount | Board records | IsSave | Matched bytes | SHA-256 |
|---|---:|---:|---:|---:|---|
| `BCNT0.SAV` | 0 | 1 | 1 | 1,649/1,649 | `ecac15c78f250f69459c9f8e5b0670ef400985245b74b3e54f26edee789508b7` |
| `BCNT1.SAV` | 1 | 2 | 1 | 2,786/2,786 | `57912183490aaea13873ca725a30b29205388fc9d62ee35f5d3a04d5efb001aa` |
| `BCNT99.SAV` | 99 | 100 | 1 | 114,212/114,212 | `c52c16f445dda8cf858d4e1443adc6e97c20fda128ca0500820222f7a11354f7` |
| `BCNT100.SAV` | 100 | 101 | 1 | 115,349/115,349 | `9b3e4a24e787461510cc99a1441573ca57ad5a2cfa18b9072ab0dcbdf115bcdc` |
| `BCNT0.ZZT` | 0 | 1 | 0 | 1,649/1,649 | `a0e993d2137df2c54a8437701806db60f26fb17a7a991f73ea25fb8f16891f47` |
| `BCNT1.ZZT` | 1 | 2 | 0 | 2,786/2,786 | `a8298b8ca21abb77e0e2335f6db87e7f6b4890bba511d2f096ed4f3113cce744` |
| `BCNT99.ZZT` | 99 | 100 | 0 | 114,212/114,212 | `8fb129c6610af5538289825441f6df7d5c944766c942efe1856de64dcfc125cc` |
| `BCNT100.ZZT` | 100 | 101 | 0 | 115,349/115,349 | `e5d00df5ede39784f5afb90c3cc6b998805eb7e85e2e4b9f7bf3335ba1e93ab1` |

The 2026-11-27 clean run exited 0 with 149 direct Pascal sessions, 73/73 configured direct Pascal-to-production comparisons, and integrated fixtures F01-F10 (238 checkpoints, 824,821 state fields, 169 logical cycles). The eight complete files account for 467,992 matching bytes and zero unexplained mismatches. `E2-02-AC002` remains partial; other fields, string/Boolean variants in other formats, RLE boundaries, and malformed/truncated cases remain open.
