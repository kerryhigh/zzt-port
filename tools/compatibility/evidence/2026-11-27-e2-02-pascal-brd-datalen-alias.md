# Direct Pascal `.BRD` shared `DataLen` alias differential

- Evidence class: **D+DP**.
- Reference: reconstructed Pascal `BoardOpen`/`BoardClose`, compiled with Turbo Pascal 5.5 and run in a fresh DOSBox-X 2026.10.01 session.
- Production: shared LibZoo external-board record import, parser, and serializer.
- Engine source revision: `b4d6b8c9b3c7769d7b6be52ce61b6393a0b1eac5`.
- Fixture: `DEMO.ZZT`, first scripted-board alias found by the probe.

`BRDALI.PAS` opens the source-produced DEMO boards using the original Pascal routines, selects the first positive-length script and a later stat that can safely be redirected, gives the later stat the earlier script pointer, then invokes the original `BoardClose`. It does not patch serialization or memory semantics. The selected case is board 2, source stat 2, target stat 3: source `DataLen=98`, target pointer aliases the source, and the captured wire `DataLen` is `-2`.

The resulting `ALIAS.BRD` is 2,027 bytes including its two-byte length prefix. Its 2,025-byte record SHA-256 is `6f9eaf8cc7b5a33dba9361a0ebc577cb51b97359656eccab79bffd84a04b6e21`; the complete file SHA-256 is `dab794eb8964f3b7c066c771d72691abaecef6a1805061c58e84284514a90e76`. The production parser/import/export accepted the Pascal record and reproduced all **2,025/2,025 record bytes**, retaining target stat 3's negative wire length `-2`. `BRDALI.OUT` contains `alias 2 2 3 98 -2` (SHA-256 `3ba1875fcb675c2b976315059eb05181702c13140686b3b06c8a6daaac92cef8`).

The run used the existing clean Pascal verifier, which recreated its staging directory and completed with exit code 0: 149 direct Pascal sessions, 71/71 direct Pascal/production comparisons, and all ten E2-10 replay fixtures. This one alias case is direct evidence for shared script-pointer serialization; it does not close AC002. Other `DataLen` values/alias layouts, count boundaries, other format fields, and malformed/truncated input matrices remain open.
