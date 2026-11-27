# E2-02 `BRDXFER` memory-context follow-up — 2026-11-27

This follow-up corrects the direct editor-transfer probe's executable memory settings and records a fresh attempt at the one-byte `.BRD` header case. It does not close E2-02 or change the frozen acceptance inventory.

## Probe setup

`tools/pascal-compat/BRDXFER.PAS` now declares `{$M 49152,163840,655360}`, matching the reconstructed game's directive in `third_party/reconstruction-of-zzt/SRC/ZZT.PAS`. The change affects only the test executable's stack and heap limits; no Pascal gameplay unit or production engine routine changed.

The direct probe compiles with Turbo Pascal 5.5 from a clean staged copy. Staged `EDITOR.PAS` supplies the transfer-menu choice and fixture name at the existing prompt boundaries and inserts a call to the original `EditorTransferBoard` inside `EditorLoop`. Staged `GAME.PAS` substitutes the post-Escape state (`TextWindowRejected := true; InputKeyPressed := #0`) at the `DisplayIOError` acknowledgement boundary so the probe does not block on modal input. The `BlockRead`, both `IOResult` checks, `DisplayIOError`, and `BoardOpen` remain the reconstructed Pascal routines. The modal key loop itself is outside this probe.

## Results

- Clean command: `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain 'build/pascal-compat-run/Z' -DosboxExecutable '<external DOSBox-X 2026.10.01 executable>'`.
- Exit code: **0**.
- Full clean run: **147/147** direct Pascal sessions, **65/65** configured Pascal/production differential invocations, and frozen F01–F10 at **238 checkpoints**, **824,821** checkpoint state-field comparisons, and **169** logical-cycle snapshots. The first-divergence negative control passed. No unexplained comparable mismatch was reported.
- The short-payload control (`SHORTPAY.BRD`: `04 00 10 20`) still produced `board 0 255 0 4 31 30 12 0`. The clean run's direct Pascal/production comparison matched the transfer outcome and seven initialized board/player fields. The 28-byte `BRDXFER.OUT` SHA-256 was `7d6bef05b479c0dfff214c3763d00cef2cbcd5711f43018d68c158ada063ad00`.
- A separate fresh DOSBox-X 2026.10.01 session used the same rebuilt executable, ZZT memory directive, staged prompt/acknowledgement hooks, `cycles=max`, and a valid 8.3 fixture named `SHORTHD.BRD` containing the single byte `04`. It timed out at **25 seconds** and produced no `SHORTHD.OUT`. This attempt is **not counted as a passing direct probe or differential**.

The DOSBox-X executable was version 2026.10.01, SHA-256 `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`. Turbo Pascal 5.5 remains an external, non-redistributed dependency.

## Compatibility status

The corrected memory context preserves the passing short-payload trace but does not make the short-header `BoardOpen` path yield a stable direct oracle result. Its final state remains source-derived; the production safe-reopen behavior remains a production-only regression and is not evidence of Pascal parity. E2-02 stays partial, and the short-header case remains release-blocking under the existing acceptance criteria.
