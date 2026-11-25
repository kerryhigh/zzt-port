# E2-02 Pascal BoardClose and WorldSave byte differential

**Date:** 2026-11-25  
**Evidence:** DP (compiled reconstructed Pascal and shared production LibZoo)  
**Status:** Partial evidence for E2-02 AC001 and the E2-03 BoardClose source routine; neither E2-02 nor E2-03 is closed.

## Fixture and comparison

`tools/pascal-compat/BOARDENC.PAS` loads the checked-in DEMO world, fills the active 60×25 board with empty tiles colored `0x70`, and calls the original reconstructed-Pascal `BoardClose`. The probe captures the resulting length-prefixed board record in staging file `BOARDENC.BRD`, then calls the original `WorldSave('BOARDENC', '.ZZT')` to create a complete saved world file.

`tools/pascal-compat/compare-pascal-boardenc.js` makes two distinct checks:

1. The Pascal-produced 289-byte board payload round-trips through the shared LibZoo `.BRD` import/parse/export routines with every record byte matching.
2. The production harness reads the same original DEMO input, opens and mutates the same active board, calls shared `zoo_board_close`, and serializes through `zoo_io_world_write`. The full output is compared with Pascal's actual `WorldSave` file.

| Output | Size | Result |
|---|---:|---|
| `BOARDENC.BRD` | 291 bytes including length prefix | SHA-256 `e97bfa7b3c33355f54bd04e8574bf5eb3e4d14ad834918d021e590ad04a7dddd` |
| Board record payload | 289 bytes | All bytes match through shared LibZoo import/export |
| `BOARDENC.ZZT` | 25,928 bytes | All bytes match shared production world serialization; SHA-256 `5ca053d06dbc8673c93c02e65c97bdf4692995bd1642d6341cc1b8ed24f6f9fa` |

A preliminary experiment compared Pascal's raw `World.BoardData` buffer with LibZoo's packed internal board buffer and showed 289 versus 261 bytes. Those buffers have different internal representations, so that mismatch was not a valid DOS file compatibility comparison and is excluded from evidence. The corrected comparison crosses the actual serialized `.ZZT` file boundary and passes byte-for-byte.

The generated `.BRD` and `.ZZT` files stay in the ignored verification staging directory. The reconstructed gameplay source remains untouched; only the standalone probe writes extra captures.

## Verification

The full `tools/pascal-compat/verify.ps1` runner completed with exit code 0 on 2026-11-25 using Turbo Pascal 5.5, DOSBox-X 2026.10.01 (SHA-256 `072c1944635ccc545a816e128625e3405feab559021e4ef60b2d512f97f754c0`), Node.js 22.19.0, and Python 3.13.7. It passed 84/84 direct Pascal sessions and 44/44 configured Pascal-to-production comparison invocations. The invocation containing this probe performs both full-byte checks above. Existing serializer, high-score, runtime, and integrated prototype checks also passed; the required E2-10 replay corpus remains 0/10.

## Limits

This gives direct evidence for one valid `.ZZT` save state and one Pascal-produced `.BRD` record. It does not complete the checked-in four-format fixture manifest, field/boundary matrix, all malformed/error cases, or equivalent direct comparisons for every `.SAV`, `.HI`, and `.BRD` operation. E2-02 AC001 remains partial; the BoardClose routine evidence covers one scenario only, and E2-02 and E2-03 remain open.
