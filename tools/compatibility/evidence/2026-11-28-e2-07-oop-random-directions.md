# E2-07 AC003: OOP random direction callers

## Result

Direct Turbo Pascal 5.5 execution and the production OOP interpreter match for the three randomized direction call sites in `OopParseDirection`. The probe exercises the actual slash-direction path through `OopExecute`, checks both `Random(2)` outcomes for each call site, and compares movement, instruction cursor, tile state, and both words of `RandSeed`.

| OOP direction | Initial `RandSeed` | `Random(2)` | Final `RandSeed` | Resulting object position |
| --- | ---: | ---: | ---: | --- |
| `RNDNS` | 0 | 0 | 1 | (10, 9), north |
| `RNDNS` | 2 | 1 | 269551627 | (10, 11), south |
| `RNDNE` | 0 | 0 | 1 | (10, 9), north |
| `RNDNE` | 2 | 1 | 269551627 | (11, 10), east |
| `RNDP NORTH` | 0 | 0 | 1 | (11, 10), east |
| `RNDP NORTH` | 2 | 1 | 269551627 | (9, 10), west |

All **66/66 captured numeric fields** matched across six cases. The regenerated `OOPRND.OUT` was 173 bytes with SHA-256 `5E53A9179A593566FB882EF4C6764A1D92997F5E2992C3749CCE290322170DE7`.

## Reproduction and oracle boundary

From the repository root, with Turbo Pascal 5.5 and DOSBox-X 2026.10.01 available locally:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 `
  -PascalToolchain 'build/pascal-compat-run/Z' `
  -DosboxExecutable '<external DOSBox-X executable>'
```

The clean run used Borland Turbo Pascal 5.5 and DOSBox-X 2026.10.01, executable SHA-256 `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`. Proprietary compiler/emulator binaries are external and not included. The untouched reconstructed `third_party/reconstruction-of-zzt/SRC/OOP.PAS` SHA-256 is `D67E00AB15A25FCD95DA92D7297AB90BAD5D1F02D55C18327B8F952295998C50`.

`OOPRND.PAS` only substitutes `VideoWriteText` with a quiet callback, allocates `IoTmpBuf` before `WorldCreate` to match startup, constructs a fixed object and empty neighboring tiles, sets `DataPos` and `RandSeed` explicitly, and captures state after calling the original `OopExecute`. It does not modify the Pascal OOP implementation or RNG semantics. The production capture uses `zoo_oop_execute`, its actual direction parser and extracted `zoo_stat_move`, with the shared `zoo_random_next` primitive.

## Evidence and remaining scope

- Direct compiled-Pascal evidence: `D`.
- Direct Pascal/production state differential: `DP`; `compare-oop-random-walk.js` reports the first differing case/field and complete rows.
- Source mapping retained: `S`.
- Production paths: `third_party/libzoo/src/libzoo/zoo_oop.c::zoo_oop_parse_direction`, `zoo_stat_move`, and `zoo_random_next`.
- Probe and harness: `tools/pascal-compat/OOPRND.PAS`, `tools/pascal-compat/compare-oop-random-walk.js`, `tools/runtime-compat/oop_label.c`.

The full clean Pascal suite passed with **152/152 direct sessions**, **77/77 configured Pascal-to-production comparisons**, and E2-10 fixtures F01–F10 at **238 checkpoints**, **824,821 state fields**, and **169 logical-cycle snapshots**; exit code 0 and no unexplained comparable mismatch. The code and test checkpoint is local commit `32da4facaa196c57975d71d0dd344a1758ced88e`, tree `4274ca294d7a4a16f3847b4023ff3d7b8cd32280`, published mirror `67f4ecb3c898b37b8279419642a51078cc5d8d38`.

These results directly cover `OOP.PAS:143`, `:145`, and `:158` for the tested tokens and both RNG branches. E2-07 AC003 remains partial: **18/29** random call sites have direct caller evidence; 11 still lack direct D/DP evidence, including `Randomize` time-seed behavior and the register-message path. Existing F06/F07 remain integrated-only support for three other call sites and are credited only under E2-10.
