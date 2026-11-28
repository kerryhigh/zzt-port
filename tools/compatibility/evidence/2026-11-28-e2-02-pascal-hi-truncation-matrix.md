# E2-02: exhaustive `.HI` prefix truncation sweep

**Date:** 2026-11-28  
**Tested source:** published `b204b95708cc0d69cac582e2b7106bf678265695` (tree `76b97997d67d1e8dfc3e3bc676f19e3b042d2ba3`); local source `7b3fe1c643e777363364a36699c42141e05ec33c`.  
**Evidence:** direct compiled Pascal (**D**) plus shared-engine differential (**DP**).  
**Inventory:** existing E2-02-AC003/AC004 only; no acceptance ID or fixture-corpus completion credit was added. E2-02 remains partial.

## Probe and controlled inputs

`tools/pascal-compat/HILOADPRB.PAS` lines 45–59 runs the original `HighScoresLoad` routine for each incomplete `.HI` record length from 0 through 1,589. Before each call, it writes that exact prefix of the seeded 1,590-byte table to `HITRUNC.HI` and restores `HighScoreList` from the same complete seed. It appends the entire resulting Pascal table to `HITRUNC.MAT`. The seed was 1,590 bytes with SHA-256 `6877D89201EB906538370007F96774757AF5A0CD17294681EF552D041C3CF4E7`.

The shared LibZoo comparator initializes a table from the same seed, calls `zoo_high_score_table_load` with each corresponding prefix marked as an unsuccessful read, captures the complete table after each call, and byte-compares it to the Pascal matrix. This exercises production table behavior and preserves the original error classification for incomplete reads.

## Result

| Artifact | Result |
|---|---:|
| Prefix lengths | 1,590: 0–1,589 |
| Pascal result matrix | 2,528,100 bytes |
| Production result matrix | 2,528,100 bytes |
| Matching bytes | 2,528,100 / 2,528,100 |
| SHA-256 for both matrices | `B084CD783C682E52904A78249DA5C7BD79337303CAB91BC0968BDDB44EF48983` |
| Unexplained differences | 0 |

Each prefix is a distinct D/DP case, but the sweep is contained within the existing high-score comparison invocation; the configured full-runner invocation count remains 75. High-score comparisons now total 1,606: one save, 1,598 loads (the previous eight plus this 1,590-prefix sweep), and seven insertion outcomes. The 256 source-derived table-model rows remain separately classified as S evidence.

The clean Turbo Pascal 5.5 / DOSBox-X 2026.10.01 suite used DOSBox-X executable SHA-256 `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`. The full runner exited 0 with 150 direct Pascal sessions (55 standard, 95 standalone), 75/75 configured Pascal-to-production differential invocations, all ten integrated replay fixtures (238 checkpoints, 824,821 state fields, 169 logical cycles), and zero unexplained comparable mismatches. The full command was `powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain 'build/pascal-compat-run/Z' -DosboxExecutable '<external DOSBox-X 2026.10.01 executable>'`; the external executable path is intentionally omitted.

The original reference `HighScoresLoad` behavior was not changed. `HILOADPRB.PAS` only adds deterministic fixture creation and capture around the existing call. Generated matrix files remain in the ignored build directory. This closes the `.HI` incomplete-prefix subcase only; other `.HI` error classes and the remaining `.ZZT`/`.SAV`/`.BRD` field and boundary cases remain outstanding.
