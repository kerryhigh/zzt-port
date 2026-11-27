# E2-02 signed `.BRD` boundary differential

Date: 2026-11-27  
Engine source revision: `a21746dd55bd1ba680a5749491be6838c241fc88`  
LibZoo local provenance: `e5d2fe7301828a360e467e20ffd88b6c759db5f5`

## Probe and comparison

The direct compiled-Pascal `BOARDENC.PAS` probe starts from `DEMO.ZZT`, keeps the board's stat count, script allocations, and `DataLen` alias relationships intact, and uses the original `BoardClose` routine to write five `.BRD` files. It assigns each value `-32768`, `-1`, `0`, `1`, and `32767` to `Board.Info.TimeLimitSec` and every stat's `StepX`, `StepY`, `Cycle`, `Follower`, `Leader`, and `DataPos`. It records the assigned values in `BOARDENC.OUT`. The existing no-op `VideoWriteText` callback suppresses screen output only; there is no gameplay or serialization semantic hook.

`compare-pascal-signed-brd.js` sends each captured record through the shared production `.BRD` import/parser. The harness checks that all seven signed fields decode to the expected signed value for every stat record, then serializes the board and compares the entire record payload byte-for-byte with Pascal. For all five cases, the time limit and six stat fields decoded correctly across four stat records, and all 289 record bytes matched (zero differences). Each Pascal `.BRD` file is 291 bytes including its two-byte length word.

| Pascal value | File | Record SHA-256 | Result |
|---:|---|---|---|
| -32768 | `SIG0.BRD` | `8b6c4f5448e9667160fa2801552afabf0137b37812eebf4e6de4680d193a0d29` | D/DP pass; 289/289 bytes |
| -1 | `SIG1.BRD` | `63b441d68edc2a138e0799d74f915ebe1d9fb153d861762b724923f29a38b78d` | D/DP pass; 289/289 bytes |
| 0 | `SIG2.BRD` | `6b86996c089ac8fb657dc35d1b730b5109f045041820f4ca287b107e0a58d2fc` | D/DP pass; 289/289 bytes |
| 1 | `SIG3.BRD` | `ba71daea6d847b1d0b2ca657b5c46137dd4a42720a71fadebcc2ebe3b720eaab` | D/DP pass; 289/289 bytes |
| 32767 | `SIG4.BRD` | `e10f557b487faedd7e383e932a38bb6f796f1b1b3d434e6dc6528dffd780a431` | D/DP pass; 289/289 bytes |

The raw pointer bytes carried in Pascal stat records are allocator-dependent and are compared only within each generated file's parse/export round-trip. Their cross-run numeric values are not a compatibility requirement. `DataLen` was intentionally kept valid because it controls script payload and alias records; its signed edge semantics remain part of the outstanding frozen E2-02 matrix.

## Verification

Clean command (the actual external DOSBox-X path is omitted):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain 'build/pascal-compat-run/Z' -DosboxExecutable '<external DOSBox-X 2026.10.01 executable>'
```

Exit code: **0**. Turbo Pascal 5.5 under DOSBox-X 2026.10.01; executable SHA-256 `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`.

- Direct Pascal sessions: **148/148** (54 standard, 94 standalone/fixture sessions).
- Configured direct Pascal/production comparisons: **65/65**, plus the new signed-boundary comparator **1/1** (five `.BRD` cases); total **66/66**.
- Integrated replay: **F01–F10, 10/10**, 238 checkpoints, 824,821 state-field comparisons, 169 logical-cycle snapshots; no unexplained mismatch.
- Runtime, serializer, Windows x64/CTest, Android compatibility, and offline Android build checks passed in the clean verification records; no physical Android device run was performed.

This adds D/DP evidence to the existing E2-02 AC002 case; it does not add an acceptance case or close AC002. Short strings, raw length bytes, Boolean variants, reserved/stale tails, other signed fields including `DataLen`, record-count edges, RLE boundaries, and applicable malformed/truncated inputs remain in the frozen acceptance matrix.
