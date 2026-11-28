# E2-07-AC001: complete normalized input-buffer matrix

**Date:** 2026-11-28  
**Acceptance case:** E2-07-AC001 only; the frozen inventory remains at 54 acceptance cases and E2-07 remains open.  
**Evidence:** direct compiled Pascal (**D**) plus direct differential (**DP**).  
**Source revisions:** local code commit `d88c5e9a3ccf72e8222ba932175f504ac334a159`, tree `446eeafc00c3985a6c474da89af4020edb62ef8e`; published mirror commit `52484d447aabc9ace8965e67d24a2a159f84ef99`. The reconstructed reference file remains unchanged at SHA-256 `7b36ff4a6c3f607db86bef210a7ed8a924b25fafa3acb13c5553702ce933bcea`, recorded in [`reference_source_hashes.sha256`](../reference_source_hashes.sha256).

## Case and oracle path

`INPUTBUF.PAS` calls the original `InputUpdate` from `third_party/reconstruction-of-zzt/SRC/INPUT.PAS`. The verifier adds only a test setter to the staged copy of `INPUT.PAS` so each controlled input queue can be supplied without changing the reference routine. The original `Copy(InputKeyBuffer, Length(InputKeyBuffer) - 1, 1)` operation remains intact.

The deterministic matrix covers every initial byte length from 0 through 255. For each length it supplies the first N values from a permutation of bytes 1–255 and captures two consecutive `InputUpdate` results. Each row compares seven fields: configured length, update stage, consumed key, residual queue count, residual queued byte (zero when empty), and movement deltas. `InputUpdate` leaves at most one queued byte after each call, so the count and byte capture the entire logical residual queue. The comparator checks all 3,584 matrix fields and reports the first differing length, stage, and field.

Separate direct cases retain coverage of empty and one-byte input, one- through four-byte bursts, four repeated identical keys, normalized extended arrow values, CP437 bytes `0x82` and `0xFF`, numeric movement, and neutral-key delta reset. A wrapped-burst case drains 253 bytes so the production ring head is 252, enqueues `ABCDEF` across the 255-byte ring boundary, then compares both updates. The direct Pascal result is `A` with the one-byte residual `E`, followed by `E` with an empty queue.

## Recorded output and result

| Oracle output | Size | SHA-256 |
|---|---:|---|
| `INPUTBUF.OUT` | 548 bytes | `1fc3eb4e65f445b474ea84f858470cb62eb0ef1c9ef78740a41efc8853cc2f6c` |
| `INPUTMAT.OUT` | 13,127 bytes | `e295f5a2e375916e174f22b1a8263b934f5410d0c0790a68e339a0474dfc0185` |

The focused comparison passed: all 256 lengths, 512 `InputUpdate` transitions, and their exact logical residual queues matched production. The wrapped burst and all pre-existing burst, movement, extended-key, and CP437 rows also matched. No new probe session or differential invocation is counted; this matrix extends the existing `INPUTBUF.PAS` direct session and configured comparator.

The full clean command exited 0 using Borland Turbo Pascal 5.5 and DOSBox-X 2026.10.01. It passed 150/150 direct compiled-Pascal sessions (55 standard and 95 standalone), all 75/75 configured Pascal-to-production comparisons, the complete runtime compatibility stage, and integrated replay fixtures F01–F10 (238 checkpoints, 824,821 state fields, 169 logical cycles). No unexplained comparable mismatch was reported. The transient `.OUT` files are regenerated under the ignored `build/pascal-compat-verification` directory; the toolchain remains external and is not redistributed.
