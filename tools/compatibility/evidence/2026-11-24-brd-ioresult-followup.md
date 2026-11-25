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

`DisplayIOError` first calls `IOResult` in its zero check and, after observing a nonzero error, calls `IOResult` again while formatting the error number. The first call consumes the runtime error, so the second call observes zero. `EditorTransferBoard` checks `DisplayIOError` once after the length word and again after its optional payload read. Therefore the short-length and short-payload paths differ: the header error is consumed before the final check, while a payload error reaches that final check. The predicted short-header `BoardOpen` path is source-derived; a controlled compiled-Pascal observation is recorded below.

## Direct editor-operation capture (D)

`tools/pascal-compat/BRDXFER.PAS` executes the actual nested `EditorTransferBoard` routine through `EditorLoop`. The staged `EDITOR.PAS` supplies only the import menu choice and filename. The staged `GAME.PAS` supplies Escape at the `TextWindowSelect` boundary so the error acknowledgement is deterministic; `DisplayIOError`'s `IOResult` checks, file reads, `BoardCreate`/`BoardOpen`, and editor redraw/cleanup still execute from the reconstructed Pascal source. Two fixtures are used: a complete length word declaring four bytes with only two payload bytes, and a one-byte length header.

The short-payload output is `board 0 255 0 4 31 30 12 0`: empty name, max shots 255, zero stats, center Player element/color 4/31, player stat at (30,12), and serialized board length zero. This confirms `BoardCreate`.

The short-header output is `board 6 41 0 21 47 30 12 319`. These captured fields match the seeded prior board (name length 6, max shots 41, zero stats, center tile 21/47, player stat at (30,12)); `BoardOpen` reads the serialized record through the prior allocation after it was freed. The exact result repeated in three fresh DOSBox-X processes with the same seeded board and allocation history. It is evidence for that trace, not a claim that every allocator history yields those bytes. No equivalent shared-engine editor-state operation is compared yet.

## Production regression evidence (P)

The serializer suite still checks that `zoo_editor_brd_import_record` rejects a partial length word and a payload shorter than its declared length. These are parser error-class checks, not a direct differential of the original editor operation. The production import's board state after failure and the Pascal editor's allocator-dependent short-header effects are not compared here.

## Verification

- Command: `tools/pascal-compat/verify.ps1 -DosboxExecutable <local DOSBox-X executable>`.
- Exit code: **0**.
- Direct compiled-Pascal sessions: **86**; configured Pascal-to-production comparisons: **42/42 passed**. The short-header final-state output repeated across three fresh DOSBox-X sessions.
- The verifier recreated its staging directory. Reference Pascal gameplay sources remained untouched; only the existing output/input hooks in the verification staging copy were used.
- The project toolchain hashes and broader clean-run results are recorded in [`2026-11-24-verification.md`](2026-11-24-verification.md).

E2-02 remains open. Both truncated editor paths are now directly captured under a seeded trace, but neither is compared against a shared engine import/error-state API. The remaining `.BRD` requirement is to model and compare the two outcomes without relying on freed host memory, then complete the frozen boundary matrix.
