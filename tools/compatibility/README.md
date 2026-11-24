# Compatibility inventory and reports

`acceptance_inventory.json` is the frozen initial inventory for E2-01 through E2-11. Its top-level acceptance cases come from the mandatory checklist criteria. Its source inventory enumerates the reconstructed Pascal routines, element callback routines, branch sites, OOP words, record fields, RNG sites, editor procedures, and input procedures required to finish routine-level mapping.

Inventory entries without explicit D/S/P/DP/I evidence remain unclassified unless their `scope_status` starts with `excluded_`, which records an explicit frontend/presentation exclusion. A source location or nearby test name is not proof of a verified mapping. The progress generator reports excluded and unclassified entries separately. Do not mark entries complete from test counts alone.

`reference_source_hashes.sha256` records the untouched reconstructed Pascal units used to create the inventory. Do not edit those source files to obtain expected outputs. Oracle instrumentation belongs in the isolated verification staging copy, and changes to that hook must be reviewable in `tools/pascal-compat/verify.ps1`.

Create the initial frozen inventory with `python tools/compatibility/freeze_inventory.py --freeze`. The script refuses to overwrite existing artifacts. Changes to the acceptance scope require a review against the “finite release blockers” and “optional future investigations” sections of `ENGINE_2.0_CHECKLIST.md`.

`latest_verification.json` records the last completed runs and explicitly separates compiled-Pascal probes, source-derived expectations, production-only regressions, direct differentials, and integrated replay fixtures. Regenerate `COMPATIBILITY_PROGRESS.md` with `python tools/compatibility/generate_progress.py` after updating that record. Keep missing counts as “not enumerated” instead of reporting zero.
