# E2-02 `.BRD` IOResult follow-up — 2026-11-24

This is a focused evidence update for E2-02. It does not close the blocker and does not establish full editor-import compatibility.

## Direct compiled-Pascal evidence (D)

The clean runner executed `tools/pascal-compat/BRDIOPRB.PAS` with Turbo Pascal 5.5 under DOSBox-X. The probe uses the reconstructed Pascal runtime's original `BlockRead` calls. It seeds the short-read destination, records the first `IOResult`, then records the next `IOResult` without modifying gameplay or file semantics.

Observed output:

```text
partial-length 100 4610 next-IOResult 0
short-payload 0 100 next-IOResult 0 16 32 165 165
```

The one-byte length read leaves the prior high byte (`0x12`) intact while replacing the low byte with `0x02`, yielding 4610 (`0x1202`). The two-byte payload read leaves its unread suffix at the seeded value `0xA5`. In both cases the second runtime `IOResult` is zero.

## Source-derived control-flow observation (S)

`DisplayIOError` first calls `IOResult` in its zero check and, after observing a nonzero error, calls `IOResult` again while formatting the error number. The first call consumes the runtime error, so the second call observes zero. `EditorTransferBoard` checks `DisplayIOError` once after the length word and again after its optional payload read. Therefore the short-length and short-payload paths differ in the reconstructed source: the header error can be consumed before the final check, while a payload error reaches that final check. This conclusion is source-derived; the actual `EditorTransferBoard` path has not yet been driven by a compiled-Pascal input-tape probe.

## Production regression evidence (P)

The serializer suite still checks that `zoo_editor_brd_import_record` rejects a partial length word and a payload shorter than its declared length. These are parser error-class checks, not a direct differential of the original editor operation. The production import's board state after failure and the Pascal editor's allocator-dependent short-header effects are not compared here.

## Verification

- Command: `tools/pascal-compat/verify.ps1 -DosboxExecutable <local DOSBox-X executable>`.
- Exit code: **0**.
- Direct compiled-Pascal sessions: **83**; configured Pascal-to-production comparisons: **42/42 passed**.
- The verifier recreated its staging directory. Reference Pascal gameplay sources remained untouched; only the existing output/input hooks in the verification staging copy were used.
- The project toolchain hashes and broader clean-run results are recorded in [`2026-11-24-verification.md`](2026-11-24-verification.md).

E2-02 remains open. The next `.BRD` requirement is a focused direct probe of the actual editor import flow with a controlled existing board, file bytes, input tape, and allocation history, followed by a comparable shared-engine state/result capture.
