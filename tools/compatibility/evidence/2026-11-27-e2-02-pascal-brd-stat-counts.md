# Direct Pascal `.BRD` stat-count boundary differential

- Evidence class: **D+DP**.
- Reference operation: reconstructed Pascal `BoardClose` in `BOARDENC.PAS`, compiled with Turbo Pascal 5.5 and run in a fresh DOSBox-X 2026.10.01 session.
- Production operation: shared LibZoo `.BRD` record import, parse, and export.
- Engine source revision: `b4d6b8c9b3c7769d7b6be52ce61b6393a0b1eac5` (engine code unchanged for this probe).
- Source constant: `MAX_STAT=150` in `GAMEVARS.PAS`.

The existing `BOARDENC.PAS` session writes four source-produced `.BRD` records with `Board.StatCount` values 0, 1, `MAX_STAT-1` (149), and `MAX_STAT` (150). The probe initializes all serialized fixed fields and pointers for stat IDs 0 through `StatCount`, and sets every `DataLen` to zero so no script heap bytes affect the record sizes.

The production differential verifies the count field, each fixed stat record, the two-byte Pascal length prefix, and all record payload bytes. Counts 0, 1, 149, and 150 serialize 1, 2, 150, and 151 stat records. All bytes match:

| Fixture | Stat count | Payload bytes | External file bytes | SHA-256 |
|---|---:|---:|---:|---|
| `STATC0.BRD` | 0 | 190/190 | 192 | `134d46bd1eb91d6d813c853556f9224590d4bd0980c1bdc29f3adbbd68191d1f` |
| `STATC1.BRD` | 1 | 223/223 | 225 | `f2c648e12e4dc53319025b87e1ec0d1a2260d1a6c434ee33a87cfd3573554408` |
| `STATC149.BRD` | 149 | 5,107/5,107 | 5,109 | `386a34a88ec61ab6a02e41e9e05bc9cdabe4ef255b2cde4c868232b53bacb21e` |
| `STATC150.BRD` | 150 | 5,140/5,140 | 5,142 | `964b033582120834a7d1032740e0648680f40083ab4d96c5b7c9e1731b8935e8` |

The full clean verifier completed on 2026-11-27 with exit code 0: 149 direct Pascal sessions (55 standard, 94 standalone), 72/72 configured direct comparisons, and integrated fixtures F01–F10 (238 checkpoints, 824,821 state fields, 169 logical cycles). This adds evidence to existing E2-02-AC002 without adding an acceptance case or closing AC002; other serialized fields, board counts, and malformed/truncated boundaries remain open.
