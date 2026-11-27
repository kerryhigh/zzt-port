# Direct Pascal `.ZZT` raw Boolean and reserved-byte differential

- Evidence class: **D+DP**.
- Reference operation: reconstructed Pascal `WorldSave` in `WORLDSAV.PAS`, compiled with Turbo Pascal 5.5 and executed in the clean DOSBox-X 2026.10.01 session.
- Production operation: shared LibZoo `.ZZT` read, board open/close, and full-file write through `world_roundtrip.wasm`.
- Engine source revision: `b4d6b8c9b3c7769d7b6be52ce61b6393a0b1eac5` (engine code unchanged for this probe).
- Acceptance case: `E2-02-AC002` (partial; no new acceptance case or fixture-corpus completion credit).

The existing `WORLDSAV.PAS` raw-Boolean cases now write each direct Pascal `WorldSave` state as both `.SAV` and `.ZZT`. Each case uses `Move` to assign raw byte values 0, 1, 2, or 255 to all seven `World.Info.Keys` Boolean fields and `World.Info.IsSave`, and writes a 14-byte reserved-field pattern repeating 0, 1, 2, and 255. The `.ZZT` operation does not reinterpret or normalize these fields.

The production comparator verifies the DOS `-1` version marker, all seven key bytes, the raw `IsSave` byte, all reserved bytes, and every byte after a complete production read/open/close/write round-trip. All four `.ZZT` files match byte-for-byte:

| File | Raw Boolean byte | File bytes | SHA-256 |
|---|---:|---:|---|
| `BLS0.ZZT` | 0 | 26,465/26,465 | `2a8b3787e0c3b7c9cac3be8e6f5a0345e39df52297703f574576a918e549e346` |
| `BLS1.ZZT` | 1 | 26,465/26,465 | `1c42af092633eeebb2f90ac15d4c0bb4f707a29341837709541a2dac5053d7ca` |
| `BLS2.ZZT` | 2 | 26,465/26,465 | `484d41d8ea8972f1f94e2273a9c8ba1d10bf2148e38e1136704bcd293cf1fdf7` |
| `BLS3.ZZT` | 255 | 26,465/26,465 | `cece6a30613ad0c19bc84d8174b59bf13bd6503ea522d9a66a0e6eb1ff9f45ba` |

The same run rechecked the paired `.SAV` files for the same values and all 14 reserved bytes. The complete clean verifier exited 0 on 2026-11-27 with 149 direct Pascal sessions, 73/73 configured Pascal-to-production comparisons, and integrated fixtures F01-F10 (238 checkpoints, 824,821 state fields, 169 logical cycles). The four `.ZZT` files account for 105,860 matching bytes and zero unexplained mismatches. Other serialized-field, string, count, RLE, and malformed/truncated-format cases remain open under AC002.
