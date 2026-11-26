# E2-10-F09 — Editor mutation, `.BRD` round-trip, and play

## Result

**Passed 2026-11-25** with direct compiled-Pascal (D), direct Pascal/production comparison (DP), and integrated replay (I) evidence. The clean Pascal/production runner exited 0. It completed 124 direct Turbo Pascal 5.5 sessions and 53/53 configured differential invocations. F01–F09 are complete in the frozen ten-fixture corpus; F10 remains pending.

F09 compares 12 checkpoints and 40,788 state fields. All state fields matched. Five ordered playfield writes (20 event fields), no logical sound events, the exact exported `.BRD`, generated stable save bytes, and loaded-file round-trip bytes matched with zero unexplained differences.

## Replay checkpoints

`EDITORIN.PAS` runs the original `EditorLoop`, exports and imports `ABCGH.BRD`, then calls `BoardEnter` and the original `GamePlayLoop` for six deterministic cycles. The staged input tape performs F4 text entry, export, import, and return to the game. Gameplay starts paused and moves upward to avoid the edited text tile placed to the player's right.

| Checkpoint | Fields | Result |
|---|---:|---|
| L0 | 3,399 | Matched |
| S0 | 3,399 | Matched |
| T1 — pause to play | 3,399 | Matched |
| C1–C6 | 3,399 each | Matched |
| T2 — post-gameplay save | 3,399 | Matched |
| T3 — loaded `.SAV` state | 3,399 | Matched |
| F0 — final state | 3,399 | Matched |

The comparator matched 40,788 total checkpoint fields with zero state differences. It also matched five ordered cell writes (20 fields), zero logical sound events, and the generated `.BRD` byte-for-byte.

## `.BRD` and save results

- Pascal's editor exported and re-imported `ABCGH.BRD`; production's `zoo_editor_brd_serialize` and `zoo_editor_brd_transfer` output matched all **324 bytes**. SHA-256: `354bc950f710944f8f1d6420c2b33fe883c167a50bdc26a36c8bbb3d011a7675`.
- The generated 1,160-byte `.SAV` matched all **750 stable serialized bytes**. The other 410 positions are classified undefined/pointer storage; there were no unexplained differences. SHA-256: `f324882f7266af58513f37fb7e2db8f1ee79d81b866ecf7b43b02d8b5e7dadea`.
- Pascal `WorldLoad` returned true and production `zoo_world_load` returned 0 for the same `.SAV`. The 1,160-byte round-trip save matched **1,152 preserved bytes**, with eight process-pointer positions classified. Its SHA-256 matches the generated save.

## Captured oracle files

The clean run generated these ignored staging artifacts; hashes make the direct-Pascal capture identifiable without committing generated build files:

| File | Bytes | SHA-256 |
|---|---:|---|
| `ABCGH.BRD` | 324 | `354bc950f710944f8f1d6420c2b33fe883c167a50bdc26a36c8bbb3d011a7675` |
| `INTEGPR.OUT` | 85,896 | `0c7d48648b671f2867d046f416240c15008df1b8e708b07700f1a1889f913cb5` |
| `INTEGSND.OUT` | 5 | `043b5ae7334040ef63298adca08912d5287ad37e66f9425316adc50d2c1ab652` |
| `INTEGCEL.OUT` | 67 | `306698cfe9ccf8ac9d785838e243f33bd6c8657e64ba740898310ea4eabcd0ba` |
| `INTEGSAV.SAV` | 1,160 | `f324882f7266af58513f37fb7e2db8f1ee79d81b866ecf7b43b02d8b5e7dadea` |
| `INTEGLOD.OUT` | 3 | `f1b2f662800122bed0ff255693df89c4487fbdcf453d3524a42d4ec20c3d9c04` |
| `INTEG2SV.SAV` | 1,160 | `f324882f7266af58513f37fb7e2db8f1ee79d81b866ecf7b43b02d8b5e7dadea` |

## Oracle and production boundaries

- Probe: [`EDITORIN.PAS`](../../pascal-compat/EDITORIN.PAS), SHA-256 `68c1c0d328c90a25945b814b6939035b892b6637b37007adb0ce2b57bd70032f`.
- Untouched reconstructed Pascal routines: `EDITOR.PAS` SHA-256 `12907752e967bf49d64a3436ed8e6f9054aaa1c5c97a54d0d4b1d37f4d4c16d1`; `GAME.PAS` SHA-256 `38ffcd9a3fd10af6ebfa7c457d957a3737bbdde69a081e3b3c3c7043aba09596`. Both match [`reference_source_hashes.sha256`](../reference_source_hashes.sha256).
- The staged `INPUT.PAS` editor-tape hook supplies deterministic menu/editor key bytes only while `EditorLoop` runs. The hook is disabled before gameplay. `VideoWriteText` and sound calls are observed for capture; the real editor and game routines remain active.
- `RandSeed=0`; the transition-table shuffle runs before `WorldCreate`; `World.Info.unk1` is set to zero because the reconstructed routine leaves it undefined and the controlled production fixture starts zero-initialized.
- The logical clock is controlled at zero and advanced by the existing cycle hook. Both sides start paused. Six queued `8` bytes exercise the original `InputUpdate` buffer behavior and unpause/move the player upward.
- Pascal executes the editor's real text-tile placement and file export/import paths. The platform-neutral production harness seeds the equivalent text tile state at `(31,12)` and passes it through the shared `.BRD` serializer/import API before `zoo_board_enter` and the real `zoo_tick` loop. LibZoo does not implement a shared keyboard-driven editor loop; this fixture therefore does not close E2-09's remaining editor mutation inventory or verify a platform editor adapter.
- Physical sound output is disabled while logical sound queue events are captured. No gameplay routine is replaced in either engine.

## Reproduction

From the repository root, with Turbo Pascal 5.5 files under `build/pascal-compat-run/Z` and DOSBox-X 2026.10.01 available externally:

```powershell
./tools/runtime-compat/verify.ps1
./tools/pascal-compat/verify.ps1 `
  -PascalToolchain build/pascal-compat-run/Z `
  -DosboxExecutable "<path-to-dosbox-x.exe>"
```

Toolchain: Turbo Pascal 5.5 (`TPC.EXE`, `TURBO.TPL`, excluded from the repository), DOSBox-X 2026.10.01 (SHA-256 `B8D6A5D0FB7C6C3BB4CE18BB3E306EED77F0865B898E262322AC94C7D11971D3`), Node.js 22.19.0, Python 3.13.7. Pascal compilation and execution use separate fresh DOSBox-X sessions for F09 to control allocator history.

## Tested source revision

- Local root commit: `080cc69bf2abfa745ecc2ad37b5d27117dba8fc2`
- LibZoo local overlay at test time: `e5d2fe7`, based on upstream LibZoo commit `7fb3e89a8467e66a4b8cbc290c708b9fa5290886`
- F09 unexplained checkpoint, `.BRD`, sound, cell, or stable-serialization differences: **0**
