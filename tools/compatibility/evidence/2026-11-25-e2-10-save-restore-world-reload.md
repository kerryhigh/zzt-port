# E2-10-F08 — Save, restore, world reload, and board change

## Result

**Passed 2026-11-25** with direct compiled-Pascal (D), direct Pascal/production comparison (DP), and integrated gameplay replay (I) evidence. The frozen F08 fixture exercises six original `GamePlayLoop` cycles, four gameplay board transitions, a `.SAV` restore, a baseline `.ZZT` reload, and the corresponding production `zoo_world_load`, `zoo_board_change`, and `zoo_world_save` calls.

The final clean Pascal runner exited 0 with 122/122 direct sessions and 52/52 configured comparisons. F01–F08 are complete; E2-10 is 8/10, with F09 and F10 pending.

## Replay checkpoints

`SAVEIN.PAS` records `L0`, `S0`, the four game-loop transitions `T1`–`T4`, each cycle `C1`–`C6`, then post-gameplay save `T2`, live mutation/board change `T3`, `.SAV` restore `T4`, `.ZZT` reload `T5`, and final saved state `F0`.

| Checkpoint | Captured fields | Result |
|---|---:|---|
| L0 | 3,399 | Matched |
| S0 | 3,399 | Matched |
| T1 | 3,399 | Matched |
| C1 | 3,399 | Matched |
| T2 (gameplay transition) | 3,416 | Matched |
| C2 | 3,416 | Matched |
| T3 (gameplay transition) | 3,465 | Matched |
| T4 (gameplay transition) | 3,465 | Matched |
| C3–C6 | 3,465 each | Matched |
| T2 (post-gameplay `.SAV`) | 3,435 | Matched |
| T3 (mutated live state) | 3,399 | Matched |
| T4 (`.SAV` restored) | 3,435 | Matched |
| T5 (baseline `.ZZT` reloaded) | 3,399 | Matched |
| F0 | 3,399 | Matched |

The comparator reported 58,251 compared fields across the 17 checkpoints with zero state differences. It separately compares ordered cell events, so the two per-snapshot cell-write counter fields are omitted from the state-field total to honor the existing output-cadence contract.

The replay matched one logical sound event (66 event fields), 6,006 effective playfield cell changes (24,024 event fields), and the save/load data below. Pascal reported 834 identical-value redraws and production reported 835; this is the same already documented clock/poll cadence variation as F05. The effective cell event sequence and engine state matched, so this remains a classified redraw-count difference rather than an unexplained gameplay mismatch.

## File and load results

- Pascal `WorldLoad` returned true for both the `.SAV` restore and the baseline `.ZZT` reload. Production `zoo_world_load` returned 0 for each.
- The generated 1,580-byte `.SAV` matched 896 stable serialized bytes; 684 undefined or process-pointer positions were classified and none differed in stable fields.
- The 1,586-byte final `.SAV` round trip matched 1,562 stable bytes; 24 process-pointer positions were classified.
- The baseline `INTEGWRL.ZZT` was 1,586 bytes. The comparator fed those exact compiled-Pascal bytes to production for the world reload.
- All eight Pascal output artifacts are hash-recorded in [`integrated_replay_fixture_manifest.json`](../integrated_replay_fixture_manifest.json); generated files remain in ignored `build/` staging and are not committed.

## Oracle setup and allowed interventions

- The probe is compiled from `tools/pascal-compat/SAVEIN.PAS` against the untouched reconstructed Pascal units, including `GAME.PAS` whose source hash remains `38ffcd9a3fd10af6ebfa7c457d957a3737bbdde69a081e3b3c3c7043aba09596` in [`reference_source_hashes.sha256`](../reference_source_hashes.sha256).
- It sets `RandSeed := 0` and runs the original `GenerateTransitionTable` before `WorldCreate`, matching ZZT startup order. It seeds `World.Info.unk1` and `pauseBlink` only where the source leaves those values undefined and the controlled production fixture starts zeroed.
- A fixed `LLLR` normalized input tape and `SoundProbeSetTime(0,0,0,30)` logical clock replace nondeterministic keyboard and wall-clock reads. The real `GamePlayLoop`, `InputUpdate`, callbacks, `WorldSave`, `WorldLoad`, and `BoardChange` remain active.
- Audio hardware output is disabled while the original `SoundQueue` calls and logical queue state are captured. `VideoWriteText` is observed for cell changes without replacing gameplay flow.
- After each successful `WorldLoad`, the fixture sets `ReturnBoardId := World.Info.CurrentBoard` before `BoardChange`, matching the title-loop restore/world-load path. An initial failed attempt omitted this assignment and was discarded as a fixture setup error; it did not change the Pascal oracle or expected engine behavior.
- Turbo Pascal compilation and executable runs use separate fresh DOSBox-X sessions to control inherited allocator history.

## Reproduction

From the repository root, with the external Turbo Pascal 5.5 files under `build/pascal-compat-run/Z` and DOSBox-X 2026.10.01 available locally:

```powershell
./tools/pascal-compat/verify.ps1 `
  -PascalToolchain build/pascal-compat-run/Z `
  -DosboxExecutable "<path-to-dosbox-x.exe>"
```

The clean run used Turbo Pascal 5.5, DOSBox-X 2026.10.01 (SHA-256 `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`), Node.js 22.19.0, and Python 3.13.7. Proprietary compiler and emulator binaries are external and are not redistributed.

## Source and implementation revision

- Local implementation revision tested: `6668babc88d5d53add6f0f1ceb4c1eac12f57dea`
- LibZoo local overlay at test time: `e5d2fe7` (upstream baseline `7fb3e89a8467e66a4b8cbc290c708b9fa5290886`)
- Probe source SHA-256: `92d3861fbf6a2a446cdd10cc0f1d72ddcaa1cc39ec8e753b2d96b8e1c4310a00`
- Unexplained state, sound, effective-cell, or stable-save mismatches: **0**
