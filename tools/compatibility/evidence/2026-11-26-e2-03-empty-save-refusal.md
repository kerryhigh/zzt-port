# E2-03-AC001 — empty `.SAV` refusal through GameTitleLoop

**Result: direct compiled-Pascal and production comparison passed for the existing zero-byte-save refusal route. E2-03-AC001 remains partial; no acceptance ID or inventory denominator changed.**

## Oracle route and fixture

`tools/pascal-compat/TITLEIN.PAS` mode `E` creates an existing zero-byte `RESTORE.SAV`, starts the ordinary title setup, queues `R`, and runs the original `GameTitleLoop` and `GameWorldLoad('.SAV')` routines. The saved file is zero bytes (SHA-256 `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`). The Pascal capture is generated at `build/pascal-compat-verification/TILEMPT/TITLFAIL.OUT` (7,113 bytes; SHA-256 `61003ce108e18dfd7bf5b7826df3801ddbda76ae93bebdbefd9115e523bcc42`). Both are transient clean-stage outputs and are not committed.

The direct compiled-Pascal run uses the installed Turbo Pascal 5.5 toolchain and DOSBox-X 2026.10.01 (SHA-256 `b8d6a5d0fb7c6c3bb4ce18bb3e306eed77f0865b898e262322ac94c7d11971d3`). The source oracle's gameplay, file-reading, and title-loop routines are unchanged. The clean staging script supplies Enter at the existing save selector and injects Escape after the short header `BlockRead` so the existing error modal can return; this captures post-modal state and does not replace `WorldLoad` or the restore logic. This UI acknowledgement is not a shared-engine comparison.

## Production comparison

The exact same empty fixture is supplied to the shared production restore path. `zoo_io_world_read` reads the complete 512-byte Pascal header into a local staging buffer before applying header fields to the world. A short header returns `ZOO_ERROR_INVAL` (`-2`), leaving the source-defined pre-load world and board state intact.

`compare-title-lifecycle.js --restore-empty` matched **3,384 of 3,386** comparable world, board, stat, key, and tile fields. It excludes field 2, LibZoo's private board-buffer length, and field 16, `InputKeyPressed`: Pascal's frontend error modal consumes Escape before its state capture, while the shared restore API returns before any frontend acknowledgement. The 3,384 compared engine fields have no unexplained differences.

## Clean verification run

The full clean Pascal/production runner completed on 2026-11-26 with exit code 0: 140 direct Turbo Pascal sessions (54 standard programs and 86 standalone sessions), 62/62 direct Pascal-to-production differential invocations, and all ten frozen integrated fixtures passing (238 checkpoints, 824,821 state-field comparisons, and 169 logical-cycle snapshots). The first-divergence diagnostic negative control passed. `tools/runtime-compat/verify.ps1` also exited 0. The empty-save comparison is one additional route within existing E2-03-AC001 and E2-02-AC001 coverage; the frozen 54-case inventory is unchanged.

The clean runner command was:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain build/pascal-compat-run/Z -DosboxExecutable <external DOSBox-X 2026.10.01 executable>
```

The Turbo Pascal compiler/runtime and DOSBox-X executable are external dependencies and are not redistributed.

## Remaining lifecycle work

This verifies an existing empty-save refusal only. E2-03-AC001 remains partial until the remaining save/return paths and the consistent successful/refused world and board transition matrix meet the frozen checklist gate. E2-02 also remains partial because its four-format serialized-field and malformed-boundary matrix is not complete.
