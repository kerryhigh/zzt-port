# E2-02 `.BRD` IOResult follow-up — 2026-11-24

This focused update records the latest direct probe and production comparison. E2-02 remains open; the frozen boundary matrix and short-header behavior are incomplete.

## Direct compiled-Pascal evidence (D)

The clean runner executes `tools/pascal-compat/BRDIOPRB.PAS` with Turbo Pascal 5.5 under DOSBox-X. The probe uses the reconstructed Pascal runtime's original `BlockRead` calls and seeds the short-read destination before recording `IOResult` results.

```text
partial-length 100 4610 next-IOResult 0
short-payload 0 100 next-IOResult 0 16 32 165 165
```

The one-byte length read leaves the prior high byte (`0x12`) intact and replaces the low byte with `0x02`, yielding 4610 (`0x1202`). The short payload leaves its unread suffix at the seeded `0xA5`. The next `IOResult` is zero in both cases.

`tools/pascal-compat/BRDXFER.PAS` also runs the actual nested `EditorTransferBoard` through `EditorLoop`. The staged editor unit supplies the import choice and fixture name at prompt boundaries. The staged game unit supplies Escape at the existing error-dialog key boundary. File reads, `DisplayIOError`, `BoardCreate`/`BoardOpen`, and editor cleanup still run from the reconstructed Pascal routines.

The short-payload fixture declares four bytes and provides two. Direct output:

```text
board 0 255 0 4 31 30 12 0
```

This reports empty name, max shots 255, zero stats, center Player element/color 4/31, player stat at (30,12), and board length zero. It confirms the editor reaches `BoardCreate`.

The older one-byte-header fixture was named `SHORTHEAD` (nine characters), so Turbo Pascal's DOS 8.3 file access could not open it; earlier apparent captures from that fixture are invalid and have been removed. A corrected eight-character `SHORTHD` attempt does not yield stable state: the direct run stalls, and an instrumented attempt reports Pascal runtime error 203 while `BoardOpen` follows the freed-buffer path. This attempt is not counted as a passing direct probe. The expected header control flow is source-derived only.

## Source-derived control-flow observation (S)

`DisplayIOError` calls `IOResult` for its zero check and calls it again while formatting a nonzero error. The first call consumes the runtime error, so the second observes zero. `EditorTransferBoard` checks the function after the length word and again after its optional payload. This predicts that a short header skips allocation and payload reading, then the final error check falls through to `BoardOpen` after the prior `BoardData` has been freed. The production API models this inferred reopen using freshly serialized bytes; direct Pascal parity for that model is unverified.

## Direct differential and production evidence (DP / P)

`tools/pascal-compat/compare-editor-brd-transfer.js` pairs the short-payload trace with the shared `zoo_editor_brd_transfer` API. It compares the transfer outcome and seven initialized fields: board name length, max shots, stat count, center tile element/color, and player stat X/Y. All seven matched the direct Pascal output. The Pascal output also records `World.BoardLen[0]`; the serializer harness emits an external board body, so that internal length is not counted as a compared field.

`tools/serializer-compat/verify.ps1` separately checks parser error classes for a one-byte length header and a payload shorter than its declared length. Those are production regressions, not end-to-end editor-state differentials. The short-header editor-state comparison remains pending.

## Verification

- Clean command: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain <Turbo Pascal 5.5 directory> -DosboxExecutable <DOSBox-X 2026.10.01 executable>`.
- Exit code: **0**.
- Direct Turbo Pascal sessions: **84/84 passed** (54 standard probe programs and 30 standalone sessions). The unsuccessful short-header attempt is excluded.
- Configured Pascal-to-production comparisons: **43/43 passed**, including the seven-field short-payload editor-state comparison.
- Additional high-score byte comparisons: **16/16 passed**; source-derived high-score model rows: **256**, counted separately.
- Integrated prototype: **9 checkpoints × 3,421 fields = 30,789 comparisons passed**. It remains a prototype, not one of the ten E2-10 fixtures.
- `tools/serializer-compat/verify.ps1`: exit code **0**.
- Android debug and instrumentation APK builds: passed offline for `arm64-v8a` and `x86_64`.
- The runner recreated its staging directory. Reconstructed Pascal gameplay source units remained untouched; probe hooks and deterministic prompt input were applied only in the staging copy.

E2-02 remains partial. The short-payload editor behavior is directly captured and compared for selected state; the short-header final state is source-derived, not directly verified. Complete the frozen malformed-read boundary matrix and resolve the shared short-header editor-state comparison without depending on freed host memory.
