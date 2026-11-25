# E2-10 first-divergence diagnostic negative control

The integrated replay comparator's first-divergence diagnostic was checked with an intentional mismatch in a disposable copy of `INTEGPR.OUT`. The test increments only field 0 (`ammo`) in the `C1` cycle row, then runs the real `compare-integrated-gameplay.js` against the unchanged production replay output, sound/cell traces, and save files.

Expected comparator child result: exit code 1, with the earliest mismatch reported as:

```text
First divergence at cycle 1, field 0 (ammo): Pascal=1, production=0; context=[ammo:1->0, gems:0->0, health:100->100]
```

The original Pascal capture remained unchanged before and after the check (SHA-256 `e302935a10fbcbabdc7d39ba44c99e2bcc9a6e05bdf7951b26c6833059f10efe`). The negative control passes only if the comparator exits with the expected mismatch and provides cycle, field name, values, and surrounding fields.

The clean Pascal runner executed this check after its normal integrated comparison and exited 0 on 2026-11-25 at root revision `05b653336b4da933737920a6ddad913aeaaadf74`. That run passed 84/84 direct Pascal sessions and 44/44 configured Pascal-to-production comparisons.

This validates diagnostic infrastructure only. It is not a compatibility mismatch, does not count as a replay fixture, and adds no completion credit to any frozen acceptance case. The replay prototype remains outside the required corpus at 0/10 fixtures.

Reproduce it with the generated artifacts after the normal Pascal replay:

```powershell
node tools/pascal-compat/check-integrated-diagnostics.js `
  tools/pascal-compat/compare-integrated-gameplay.js `
  build/pascal-compat-verification/INTEGPR.OUT `
  build/runtime-compatibility/integrated_gameplay.wasm `
  build/pascal-compat-verification/INTEGSND.OUT `
  build/pascal-compat-verification/INTEGCEL.OUT `
  build/pascal-compat-verification/INTEGSAV.SAV `
  build/pascal-compat-verification/INTEGLOD.OUT `
  build/pascal-compat-verification/INTEG2SV.SAV `
  build/pascal-compat-verification/INTEGPR_DIAG_MUTATION.OUT
```
