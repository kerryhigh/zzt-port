# E2-07-AC003: seeded RNG primitive and call-site inventory

**Date:** 2026-11-28  
**Acceptance case:** E2-07-AC003 only; it remains partial. The frozen inventory remains at 54 cases.  
**Evidence:** direct compiled Pascal (**D**), direct differential (**DP**), and source-derived call-site mapping (**S**).  
**Tested code:** local commit `820ce38e3139ae8810998eafe084ab5bc477393f`, tree `2f63a92fbc7f2f0f3fb28b8a1646fc2ed718e7c7`; published code mirror `853ef9de7bc9f8e1ae374d5121d294bca56fb473`. The untouched reconstructed source hashes are recorded in [`reference_source_hashes.sha256`](../reference_source_hashes.sha256).

## Direct seeded vector

`RNGVEC.PAS` assigns `RandSeed` directly and calls Turbo Pascal 5.5 `Random` for 16 seeds and 14 ranges, producing 224 rows. Seeds include zero, signed high-bit values, wrap edges, and seeds already used by gameplay probes. Ranges include 0, 1, small movement ranges, existing gameplay ranges, 5,000, and the maximum positive Pascal `Integer` value 32,767. Each row records its seed/range identity, the returned value, and both 16-bit halves of the resulting 32-bit `RandSeed`.

`compare-rng-vector.js` compares all six numeric columns for all rows against the production `zoo_random_next` primitive. All **1,344/1,344 numeric fields** matched, including `Random(0)` returning zero while advancing the seed. The direct Pascal output is `build/pascal-compat-verification/RNGVEC.OUT`, 5,235 bytes, SHA-256 `300DFDD9F8C62887731B5D5E9DB88FB51F2D3C068CF643CD0FA98D7488B2C980`. The output is generated during verification and is not committed as a binary fixture.

The direct probe sets only the documented `RandSeed` input and writes results to a text file. It does not patch Turbo Pascal's `Random`, alter reference gameplay routines, or change expected output to match production.

## Random call-site coverage

The frozen source inventory maps **29/29** `Random` call sites to reconstructed Pascal source. Existing direct gameplay probes supply D/DP coverage for **15/29** sites: `GAME.PAS:1276,1279,1289` and `ELEMENTS.PAS:90,113,135,136,142,151,211,220,221,426,427,785`. The specific Pascal probe and production implementation mapping is recorded on each stable `E2-07-RNG-*` inventory row. The matching element fixtures are `LIONPRB`, `TIGRPRB`, `RUFFPRB`, `CENTIPR`, `SPINGUN`, and `SHARKPRB`.

The remaining **14** sites are still source-derived only: `GAME.PAS:140,1515,1564,1699`; `ELEMENTS.PAS:205,208,1290`; `OOP.PAS:143,145,158`; and `SOUNDS.PAS:118,121,128,130`. These require direct probes for caller range, conditional call order, state after the call, and resulting behavior. The Centipede direct fixture does not reach the short-circuited `ELEMENTS.PAS:205` or `:208` calls. No call-site coverage is inferred from the primitive vector alone.

`GAME.PAS:1699` (`GamePrintRegisterMessage`) currently has no matching production implementation mapped in LibZoo; whether this is an intentionally unsupported path or a material discrepancy remains unresolved. The four `SOUNDS.PAS` drum-table calls build randomized Pascal note data while LibZoo holds a static drum table; those values and RNG-state effects have not yet been directly reconciled. `Randomize`/initial time-seed behavior is also outside this fixed-seed vector and remains pending controlled BIOS-time verification.

## Full-suite result

The clean Turbo Pascal 5.5 / DOSBox-X 2026.10.01 verifier passed with the new vector in its normal staged run: **151/151 direct Pascal sessions** (56 standard, 95 standalone) and **76/76 configured Pascal-to-production comparisons**, exit code 0. E2-10 fixtures F01–F10 remained at 238/238 checkpoints, 824,821 state fields, and 169 logical-cycle snapshots; there were zero unexplained comparable mismatches. DOSBox-X executable SHA-256: `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`. The proprietary Turbo Pascal 5.5 toolchain remains external and is not redistributed.

The direct suite command was:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/pascal-compat/verify.ps1 -PascalToolchain 'build/pascal-compat-run/Z' -DosboxExecutable 'C:\path\to\dosbox-x.exe'
```

The full seed list, range list, results, output hash, per-site mappings, status counts, and pending call-site IDs are maintained in `tools/compatibility/latest_verification.json` and `tools/compatibility/acceptance_inventory.json`. AC003 remains open until every listed call site and the relevant time-seed path have direct evidence.
