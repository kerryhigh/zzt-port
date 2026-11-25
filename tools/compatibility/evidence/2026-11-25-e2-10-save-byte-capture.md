# E2-10 generated save-byte capture

**Date:** 2026-11-25  
**Evidence:** DP (compiled reconstructed Pascal compared with production LibZoo)  
**Status:** Partial evidence for E2-10 AC001 and AC004; does not close either case or E2-10.

## Scope and setup

The existing `INTEGPRB` gameplay prototype now calls the reconstructed Pascal `WorldSave` and production `zoo_world_save` after the same six-cycle gameplay state. It compares the produced `.SAV` while retaining the existing state checkpoints, sound-request capture, and ordered cell-write capture. The Pascal oracle source remains untouched; `verify.ps1` applies instrumentation only to a clean staging copy. The reconstructed source revision and file hashes remain recorded in `../reference_source_hashes.sha256`.

The one test-only setup normalization is `World.Info.unk1 := 0`. `WorldCreate` leaves this word undefined, while the production fixture starts from zeroed storage. No gameplay or serializer routine was changed in the Pascal oracle. Broader and targeted Pascal `FillChar` attempts were removed after the direct DOSBox-X probe stalled following the first cycle; the passing run uses only the single documented word normalization.

## Captured result

The compiled Pascal run generated `INTEGSAV.SAV` (961 bytes, SHA-256 `4e781a728d4673d732fe1e34724f979236b603e79451a9edc8644b89f1c02aee`). Production `zoo_world_save` returned `0`. The comparator found no difference in the 565 stable byte positions. It explicitly classifies 396 positions as raw bytes from undefined Pascal memory or process-specific pointers; 327 of those happened to differ and 69 happened to match. The comparator reports the first mismatch outside those ranges with byte offset and surrounding values, and rejects any unclassified trailing bytes or overlapping classifications.

The 396 classified positions belong to these fields:

- Empty payload tails of the world name and ten world flags.
- `WorldInfo.unkPad`.
- Empty payload tails of the board name and board message.
- Uninitialized start-player coordinates.
- `TBoardInfo.unkPad`.
- Pascal stat data-pointer bytes and unused stat pointer/reserved bytes.

Before the documented `unk1` normalization, the first raw discrepancy was the undefined word at header offset 26. The normalization makes this fixture's setup deterministic; it is not evidence that DOS initializes this word to zero.

The integrated replay still matched ten named checkpoints and 34,744 state fields, two logical sound requests / 20 fields, and nine ordered cell writes / 36 fields. The clean full runner completed with exit code 0: 84/84 configured direct Pascal sessions, 43/43 configured Pascal-to-production comparisons, and runtime compatibility passed. The direct replay differential command includes `INTEGSAV.SAV` as its fifth input.

## Limits and follow-up

This is one synthetic new-world save prototype, not one of E2-10's ten required fixtures. It does not yet compare loaded-file byte preservation, save/restore/load and board-change paths across scenarios, or a Pascal save error outcome. `WorldSave` is a Pascal procedure and exposes no return value; file creation was checked, but its I/O error state is not captured. Those requirements remain open. No requirement or acceptance denominator was changed, and AC001/AC004 remain partial.

The byte mask applies only to fields proven undefined by this fresh `WorldCreate` path. It must not be reused for bytes read from an existing file. Future fixtures must compare defined fields, string lengths/content, scripts, layout, and all other stable serialized bytes exactly while reporting each allowed undefined range.
