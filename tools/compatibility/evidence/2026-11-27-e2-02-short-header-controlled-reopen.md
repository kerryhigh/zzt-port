# E2-02 short-header editor transfer: controlled reopen

Date: 2026-11-27
Production source revision: `7a75959a736ad8044c1eb9b648721b4d8edd1348`
Evidence: **D + DP** for the listed controlled setup; E2-02 remains partial.

## Question and result

`EditorTransferBoard` reads a one-byte `.BRD` length word into a buffer that has just been freed by `BoardClose`. The prior attempt had no usable output, leaving the source-derived reopen behavior without a direct differential. A controlled allocation history now allows the original nested Pascal routine to finish and gives a repeatable final capture for the byte `03` case.

The valid DOS 8.3 fixture `SHORTHD.BRD` contains one byte, `03` (SHA-256 `084fed08b978af4d7d196a7446a86b58009e636b611db16211b65a9aadff29c5`). The original Pascal output is:

```text
board 6 41 0 21 47 30 12 259 0 0 0 0
```

The captured fields are board-name length, max shots, stat count, the seeded center tile element/color, player coordinates, visible `World.BoardLen[0]`, and four neighbor links. The Pascal result preserves the prior `BEFORE` board record as the reopen source, leaves the visible length at 259 (`0x0103`), and clears all four neighbor links. `BRDHDR.OUT` is 38 bytes with SHA-256 `5aa640be916c4a7f2c4e32dda24cf081902743ee85ca0ce1a219e959e00786f5`.

The shared-engine comparison reports `BRDXFER short-header-controlled-reopen: outcome 1; 8 selected state fields match (DP).` It also compares the board name bytes and all four neighbor values. The short-payload control still reports `board 0 255 0 4 31 30 12 0 0 0 0 0` and matches its eight selected production fields; `BRDXFER.OUT` is 36 bytes with SHA-256 `f88b2af33496a6e85a415bf9103252780d8e3f4a697af7d537ecfeedb3426b4a`.

## Direct Pascal setup and interventions

Probe: `tools/pascal-compat/BRDXFER.PAS`, compiled against the reconstructed Pascal units by Turbo Pascal 5.5 and executed in DOSBox-X 2026.10.01. The probe uses the original ZZT memory directive `{$M 49152,163840,655360}`.

The probe seeds a prior board with name `BEFORE`, max shots 41, four nonzero neighbor links, and a solid center tile; it serializes that board with `BoardClose`. For the `SHORTHD` case it first allocates a 60,000-byte heap sink in `World.BoardData[1]`. The staged `EDITOR.PAS` frees that sink immediately after the original `BoardClose` within the `EditorTransferBoard` path. This controlled allocation history keeps the freed BoardData bytes readable while error-window text allocations use the sink. The stage also supplies the import menu choice and filename at their existing prompts, and stages only the post-Escape state at the existing `TextWindowSelect` acknowledgement. The original `EditorTransferBoard`, file I/O, `IOResult` handling, `DisplayIOError` through its acknowledgement boundary, `BoardOpen`, and editor cleanup execute from reconstructed Pascal source. No gameplay semantics in the reference source are modified.

The runner creates the same single-byte `SHORTHD.BRD` fixture in a fresh verification directory and captures `BRDHDR.OUT`; this case is part of the 148-session clean runner. No output from the earlier uncontrolled allocator history or the invalid nine-character `SHORTHEAD` fixture is counted.

## Production mapping

`tools/pascal-compat/compare-editor-brd-transfer.js` invokes the shared `zoo_editor_brd_transfer` implementation and checks the returned outcome, visible board length, serialized board state, board name, and neighbor values. `third_party/libzoo/src/libzoo/zoo_editor_brd.c` now retains the pre-import closed-buffer length as its safe parse window, models the Pascal low-byte overwrite of the visible length (preserving its high byte with defined conversion), parses the reopened board through `zoo_board_open`, and restores the visible partial length. It does not read freed host memory or rely on C undefined behavior.

The focused comparator and the complete clean verifier both passed. This DP result establishes parity for this input under this documented DOS allocator setup. It does not establish identical allocator reuse for every DOSBox-X run or hardware configuration, nor does it close the broader `.BRD` error and truncation matrix.

## Clean-run and remaining acceptance work

Command (external executable path redacted):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain 'build/pascal-compat-run/Z' -DosboxExecutable '<external DOSBox-X 2026.10.01 executable>'
```

Exit code 0. The run passed 148 direct Pascal sessions (54 standard, 94 standalone), 65/65 configured Pascal/production differential invocations, all ten frozen integrated fixtures (238 checkpoints, 824,821 state-field comparisons, 169 logical-cycle snapshots), and the first-divergence negative control. There were no unexplained comparable mismatches. The DOSBox-X executable SHA-256 was `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`; Turbo Pascal 5.5 and DOSBox-X are external tools and are not redistributed.

The acceptance inventory is unchanged at 54 cases. E2-02 AC003/AC004 gain this direct, controlled header case, but every E2-02 acceptance case remains `partial`. The remaining release-blocking work includes the complete per-field and error-boundary fixture matrix for `.ZZT`, `.SAV`, `.BRD`, and `.HI`; truncation at each applicable length/payload boundary; untouched-buffer suffix comparisons; and the other required seeded D/DP pairs. Other allocator histories are optional unless new evidence shows a material compatibility defect; this result is not a claim of cross-environment freed-memory determinism.
