# E2-03-AC001 — missing world reload preserves live state

**Result: direct compiled-Pascal and production comparison passed.** This adds evidence to the existing frozen AC001 case; it does not create a new acceptance ID or close the lifecycle case.

## Compared path

`tools/pascal-compat/MISSLOAD.PAS` calls the reconstructed `WorldLoad(World.Info.Name, '.ZZT', false)` with the selected world named `MISSFILE` and no corresponding `.ZZT` file. The probe seeds the live world metadata, board stat, and a tile before the call. It then applies the title path's `ReturnBoardId := World.Info.CurrentBoard` assignment and captures the result plus the selected world, board, stat, and tile fields.

The production side starts at the shared `zoo_world_play` entry point used by the title P action. With `IsSave` set, that API calls `zoo_world_reload`; its missing-file error must return before the engine starts a player session. `tools/runtime-compat/world_reload.c` links the production `zoo.c` and `zoo_game.c`, and its fake I/O driver reports the requested `MISSFILE.ZZT` as missing. The harness captures the production error code and the same live-state fields. The fake driver controls only the filesystem result; it does not replace the reload or play-entry implementation.

## Oracle controls and result

The probe calls the original Pascal `WorldLoad` routine unchanged, which is the source operation used by the title P branch when `IsSave` is set. This is a direct comparison of that reload decision and the production `zoo_world_play` failure return; it does not claim to replay the entire `GameTitleLoop`. At the existing video-output boundary, `QuietWrite` suppresses screen writes. `InputProbeSetKeyBuffer` supplies Escape solely to acknowledge the missing-file modal prompt. World setup values are explicit; no RNG, clock, allocator, or gameplay behavior is replaced. The Wasm harness supplies inert imports for services reachable only after a successful reload; the missing-file error returns before those imports are called.

Direct Turbo Pascal 5.5 capture `MISSLOAD.OUT` was 51 bytes with SHA-256 `CA813E093836052530ADE37C5D89B36C592FAA8819A3792FDCF16DDEBF5C45E8`:

```text
0 0 0 0 1 17 23 37 2 123 43 777 0 31 11 7 21 46 1
```

Pascal returned false; production `zoo_world_play` returned `ZOO_ERROR_INVAL` (`-2`) before its play-start branch. The comparator matched all 18 remaining world/board fields, including preservation of the selected world name and seeded board content. The clean full runner reported 138/138 direct Pascal sessions (54 standard, 84 standalone) and 60/60 direct differential invocations: the previous 59 comparisons plus this explicit missing-world comparison. The integrated F01–F10 corpus and runtime suite also passed; no unexplained difference was reported.

The capture is generated under `build/pascal-compat-verification/` and is not committed. This verifies one missing-file reload result and state-preservation route only. Other save/return outcomes and the consistent successful/refused world and board transition matrix remain mandatory for E2-03-AC001.
