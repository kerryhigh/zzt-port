# Direct Pascal `.SAV` short-string stale-tail differential

- Evidence class: **D+DP**.
- Reference operation: reconstructed Pascal `WorldSave`, compiled with Turbo Pascal 5.5 and run in a fresh DOSBox-X 2026.10.01 session.
- Production operation: shared LibZoo `.SAV` read and save round-trip.
- Engine source revision: `b4d6b8c9b3c7769d7b6be52ce61b6393a0b1eac5` (engine code unchanged for this probe).
- Input world: source-produced `DEMO.ZZT`; no Pascal gameplay or serialization semantics were patched.

`WORLDSAV.PAS` first assigns a 58-byte `A`-through-`Z` repeating pattern and then a one-byte value to each of four short strings before calling the original `WorldSave`:

- `World.Info.Name` (capacity 20): length 1, payload `N` followed by the prior `BCDEFGHIJKLMNOPQRST` tail.
- `World.Info.Flags[1]` (capacity 20): length 1, payload `F` followed by that prior tail.
- Current board 1 `Board.Name` (capacity 50): length 1, payload `B` followed by the prior 49-byte tail.
- Current board 1 `Board.Info.Message` (capacity 58): length 1, payload `M` followed by the prior 57-byte tail.

The production comparator checks each length and raw payload tail, locates the actual current board from the serialized header, then reads and saves the complete Pascal fixture. The generated `SAVTAIL.SAV` is 26,465 bytes (SHA-256 `c3cbcc6a858faa7eeee78f7d664256dc1cb9d0f0fc8cdde049a376a076e2686f`). All four stale-tail payloads and all **26,465/26,465 file bytes** matched production.

The clean verifier completed on 2026-11-27 with exit code 0: 149 direct Pascal sessions, 71/71 configured direct comparisons, and integrated fixtures F01–F10 (238 checkpoints, 824,821 state fields, 169 logical cycles). This adds direct `.SAV` stale-tail coverage to AC002 but does not close it; other serialized fields, count boundaries, and applicable malformed/truncated inputs remain open.
