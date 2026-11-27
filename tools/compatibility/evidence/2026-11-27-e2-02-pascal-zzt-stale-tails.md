# Direct Pascal `.ZZT` short-string stale-tail differential

- Evidence class: **D+DP**.
- Reference operation: reconstructed Pascal `WorldSave`, compiled with Turbo Pascal 5.5 and run in a fresh DOSBox-X 2026.10.01 session.
- Production operation: shared LibZoo `.ZZT` read and write round-trip.
- Engine source revision: `b4d6b8c9b3c7769d7b6be52ce61b6393a0b1eac5` (engine code unchanged for this probe).
- Input world: source-produced `DEMO.ZZT`; no Pascal gameplay or serialization semantics were patched.

The existing `WORLDSAV.PAS` session assigns a 58-byte `A`-through-`Z` repeating pattern and then a one-byte value to four short strings: `World.Info.Name`, `World.Info.Flags[1]`, the current `Board.Name`, and `Board.Info.Message`. It writes the same state once with `IsSave=true` as `SAVTAIL.SAV`, then sets `IsSave=false` and invokes the original `WorldSave` to write `ZZTTAIL.ZZT`.

The production comparator confirms the raw `.ZZT` header `IsSave` byte is 0, checks every string length and retained payload tail, locates the current board from the serialized header, then reads and writes the complete Pascal fixture. `ZZTTAIL.ZZT` is 26,465 bytes (SHA-256 `6055e1b0b25bf58af0bd197a4a72f01101255b87c742ae2a2dc585e1e95895a0`). All four stale-tail payloads and all **26,465/26,465 file bytes** matched production. The related `.SAV` output keeps `IsSave=1` and also matched all 26,465 bytes.

The clean verifier completed on 2026-11-27 with exit code 0: 149 direct Pascal sessions (55 standard, 94 standalone), 71/71 configured comparisons, and integrated fixtures F01–F10 (238 checkpoints, 824,821 state fields, 169 logical cycles). This adds `.ZZT` stale-tail evidence to existing AC002 without adding an acceptance case or closing AC002; the rest of the frozen format/error matrix remains open.
