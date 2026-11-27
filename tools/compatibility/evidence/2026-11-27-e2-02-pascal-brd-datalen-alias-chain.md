# Direct Pascal `.BRD` chained `DataLen` alias compatibility

- Evidence class: **D+DP**.
- Acceptance case: `E2-02-AC002` (the alias-chain layout is verified; AC002 remains partial).
- Source behavior: reconstructed `GAME.PAS::BoardClose` scans every earlier stat for a shared `Data` pointer and overwrites `DataLen` on every match. The final wire reference therefore names the last earlier matching stat.
- Direct probe: `tools/pascal-compat/BRDALI.PAS` first creates the existing board 2/stat 2 to stat 3 alias (`DataLen=98`, wire `-2`), reopens that source board through original Pascal `BoardOpen`, then points stat 4 at the same script allocation. Its capture is:

  ```text
  alias 2 2 3 98 -2
  chain 2 2 3 4 98 3 -3
  ```

- `ALIAS.BRD` and `ALIAS2.BRD` each contain a 2,025-byte record behind a two-byte length prefix. The original alias record is compared again; for the chain case, Pascal stat 3 carries `DataLen=-2`, and stat 4 carries `DataLen=-3`, referencing the latest earlier matching stat 3.
- Production differential: `tools/pascal-compat/compare-pascal-brd-alias.js` checks the source and alias indices, each wire `DataLen`, the full Pascal record, and the full shared LibZoo parse/export result. Both records matched byte-for-byte: **4,050/4,050 record bytes**, with zero differences. `ALIAS2.BRD` is 2,027 bytes; external-file SHA-256 `b3a0b264d99f96b275c322c5fc87e6617aeece5279715b05ecfdb9bc1d7f9740`; record SHA-256 `c3db0d1a68008fa1e54fee18eabd74b85a6359a8706a58d0e009c72d73e8ed08`.
- Clean Turbo Pascal 5.5/DOSBox-X 2026.10.01 verification on 2026-11-27 exited 0: 149 direct Pascal sessions, 74/74 configured direct Pascal/production differential invocations, and E2-10 F01-F10 (238 checkpoints, 824,821 state fields, 169 logical-cycle snapshots). No unexplained mismatch was reported.
- The probe changes only its explicit deterministic stat-pointer setup and uses original `BoardOpen`/`BoardClose`; no Pascal gameplay or serializer semantics are patched. Pointer bytes are compared as serialized bytes, not as host-pointer equality.
- This extends the existing AC002 alias evidence without adding an acceptance ID or integrated fixture. Other `DataLen` boundary/alias layouts, serialized fields, malformed/truncated boundaries, and untouched-buffer comparisons remain open.
