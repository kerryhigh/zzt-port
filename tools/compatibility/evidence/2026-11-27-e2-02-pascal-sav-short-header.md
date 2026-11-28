# E2-02: one-byte-short `.SAV` header restore

**Date:** 2026-11-27  
**Published code commit:** `e63565ef15e59d29589fab34cfafd7b6b1e77479` (tree `9c6a98a7873f32f99c41d4e2a1e8ea193ccd7613`)  
**Evidence:** direct compiled Pascal (**D**) plus shared-engine differential (**DP**).  
**Inventory:** existing E2-02-AC002/AC003/AC004/AC005 only; no acceptance ID or fixture-corpus credit was added. E2-02 remains partial.

## Case and oracle path

`TITLEIN.PAS` mode `T` writes an existing `RESTORE.SAV` containing 511 zero bytes, then enters the reconstructed `GameTitleLoop` and requests the original `R` restore route. `WorldLoad` unloads the current board and performs its original 512-byte `BlockRead`; the one-byte-short read follows the original DOS I/O-error dialog and returns failure. No Pascal loader, serializer, or gameplay semantics were changed.

The verification staging hook injects Escape only at the existing `TextWindowSelect` acknowledgement boundary so the probe can capture state after the original refusal. The production side passes the same file bytes through `run_integrated_title_restore_failure_checks` and the shared `zoo_world_restore` path. It returns `ZOO_ERROR_INVAL (-2)`, matching the Pascal refusal result.

## Recorded result

| Artifact | Size | SHA-256 |
|---|---:|---|
| `RESTORE.SAV` | 511 bytes | `117985087e92cba0e8fdd6b35599d4ef451dda3ed40c865cad00b01708721666` |
| `TITLFAIL.OUT` | 7,113 bytes | `61003ce108e18dfd7bf5b7826df3801ddbfda76ae93bebdbefd9115e523bcc42` |

The comparator compared 3,384 of 3,386 state fields. It excluded only Pascal `World.BoardLen[0]` versus LibZoo's private board-buffer length and `InputKeyPressed`, which contains the Escape consumed by the DOS error dialog. All other world, board, stat, key, and tile fields matched; production returned `ZOO_ERROR_INVAL (-2)`.

The clean Turbo Pascal 5.5/DOSBox-X run exited 0, passed the 10/10 frozen integrated fixtures (238 checkpoints, 824,821 fields, 169 logical cycles), and reported no unexplained comparable mismatch. The runner's direct-session total was corrected from its old hard-coded 149 to 150 (55 standard plus 95 standalone) to include this additional title replay; this accounting-only edit did not alter probe execution or expected results. The new comparator is the 75th configured Pascal-to-production differential (74 existing comparisons plus this one).

**Toolchain:** Borland Turbo Pascal 5.5 and DOSBox-X 2026.10.01. The Turbo Pascal compiler and DOSBox-X executable remain external, non-redistributed dependencies. The staged source preserves the untouched reconstructed Pascal gameplay routines; the only interventions are deterministic fixture creation and the documented modal Escape input.

This verifies the 511-byte header refusal boundary. Other `.ZZT`/`.SAV` field boundaries and malformed/truncated reads, complete per-field fixtures, and untouched-buffer comparisons remain open under E2-02.
