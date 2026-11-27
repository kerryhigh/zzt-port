# Direct Pascal `.BRD` chained `DataLen` alias compatibility

- Evidence class: **D+DP**.
- Acceptance case: `E2-02-AC002` (the alias-chain layout is verified; AC002 remains partial).
- Source behavior: reconstructed `GAME.PAS::BoardClose` scans every earlier stat for a shared `Data` pointer and overwrites `DataLen` on every match. The final wire reference therefore names the last earlier matching stat.
- Direct probe: `tools/pascal-compat/BRDALI.PAS` first creates the existing board 2/stat 2 to stat 3 alias (`DataLen=98`, wire `-2`), reopens that source board through original Pascal `BoardOpen`, then points stat 4 at the same script allocation. Its capture is:

  ```text
  alias 2 2 3 98 -2
  chain 2 2 3 4 98 3 -3
  loaded 2 2 3 4 98 98 98 98 1 1
  ```

- `ALIAS.BRD` and `ALIAS2.BRD` each contain a 2,025-byte record behind a two-byte length prefix. The original alias record is compared again; for the chain case, Pascal stat 3 carries `DataLen=-2`, and stat 4 carries `DataLen=-3`, referencing the latest earlier matching stat 3.
- Direct load verification: after `BoardClose` creates `ALIAS2.BRD`, original Pascal `BoardOpen` reloads it and confirms stats 2, 3, and 4 have `DataLen=98`, with stat 3 sharing stat 2's pointer and stat 4 sharing stat 3's pointer. The direct capture is `loaded 2 2 3 4 98 98 98 98 1 1`.
- Production differential: `tools/pascal-compat/compare-pascal-brd-alias.js` checks source/alias indices, each wire `DataLen`, the direct Pascal loaded pointer topology, production parser pointer topology, both full Pascal records, and both shared LibZoo parse/export results. Both records matched byte-for-byte: **4,050/4,050 record bytes**, with zero differences. In the latest clean run, `ALIAS.BRD` was 2,027 bytes (external SHA-256 `eb330dfc5d84886b0bbe78767de472b91c17e00286659a2dba5c4109c9ced08`, record SHA-256 `6883bb7774211b8b726ee8e2c14ec31517bf8cfb10efd34025c601b9cda3cf42`); `ALIAS2.BRD` was 2,027 bytes (external SHA-256 `9a0ea05ec3bf38391a7bc73747039482f4592b1e78e7105eeb840cb7739d637c`, record SHA-256 `dcd5813da9011391bee58c5b2fb4121abc6f04fef4e3714ddae04488b00deecf`). `BRDALI.OUT` is 74 bytes; SHA-256 `ecf8f70e55a4d53f860023810b15974f5e3a2ced79a85c9b4d6f1fd9b9772f73`.
- Clean Turbo Pascal 5.5/DOSBox-X 2026.10.01 verification on 2026-11-27 exited 0 after this load check: 149 direct Pascal sessions, 74/74 configured direct Pascal/production differential invocations, and E2-10 F01-F10 (238 checkpoints, 824,821 state fields, 169 logical-cycle snapshots). No unexplained mismatch was reported.
- The probe changes only its explicit deterministic stat-pointer setup and uses original `BoardOpen`/`BoardClose`; no Pascal gameplay or serializer semantics are patched. Numeric pointer bytes inside `TStat` are process/allocation-history dependent; their hashes are reported for this run, and same-run records compare as opaque serialized bytes rather than host-pointer equality.
- This extends the existing AC002 alias evidence without adding an acceptance ID or integrated fixture. Other `DataLen` boundary/alias layouts, serialized fields, malformed/truncated boundaries, and untouched-buffer comparisons remain open.
