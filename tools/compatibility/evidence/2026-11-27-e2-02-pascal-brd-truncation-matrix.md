# E2-02: Direct Pascal `.BRD` truncation boundaries

**Date:** 2026-11-27  
**Published source commit tested:** `07cbb562b392ac53361575210386c80098407062` (tree `83b2b5a569297876d8abed705936e6ee6ea43990`)  
**Evidence:** low-level reads are **D**; editor-transfer comparisons are **D+DP**; the standalone production record rejection tests are **P**.  
**Inventory:** existing E2-02 cases only; the frozen 54-case inventory is unchanged. AC002/AC003/AC004 remain partial.

## Direct `BlockRead` observations

`BRDIOPRB.PAS` seeds the two-byte length destination with `$1234` and the four-byte payload destination with `$A5`, then invokes the original Pascal `BlockRead` operations. No gameplay, file-read, or serializer semantics are patched. The DOSBox-X / Turbo Pascal 5.5 output (`BRDIOPRB.OUT`, 294 bytes, SHA-256 `4a1f7296357c1c727cc8116a4ae49601f486a37d3b8a63abb77a294aa5601890`) was:

```text
empty-length 100 4660 next-IOResult 0
partial-length 100 4610 next-IOResult 0
short-payload 0 0 100 next-IOResult 0 165 165 165 165
short-payload 1 0 100 next-IOResult 0 16 165 165 165
short-payload 2 0 100 next-IOResult 0 16 32 165 165
short-payload 3 0 100 next-IOResult 0 16 32 48 165
```

For an empty length record, I/O error 100 leaves the seeded word at 4660 (`$1234`). A one-byte `$02` length record yields 4610 (`$1202`): the byte read replaces only the low byte, while the high byte stays `$12`; the next `IOResult` is zero. For a complete length word declaring four payload bytes, zero through three available payload bytes each produce error 100. The available bytes are copied and the unread suffix retains `$A5` in this explicitly seeded direct-probe buffer. This suffix observation is D evidence only; the editor's production import discards an incomplete payload and has no corresponding portable live buffer to compare.

## Original editor transfer against the shared production operation

`BRDXFER.PAS` enters the reconstructed `EditorTransferBoard` path with prompt values injected at existing prompt boundaries. The staged `GAME.PAS` only supplies the Escape response at the error acknowledgement boundary. The probe uses ZZT's `{$M 49152,163840,655360}` memory limits. For the empty and one-byte header tests it allocates a 60,000-byte heap sink, and staged `EDITOR.PAS` frees that sink immediately after the original `BoardClose`; this controls the relevant DOS allocator history without changing the reference gameplay or file-read routine.

The six-case comparator `compare-editor-brd-transfer.js` exercises:

| Case | Fixture bytes | Pascal outcome / state | Direct production comparison |
|---|---|---|---|
| Empty header | `EMPTY.BRD`, 0 bytes | Reopens the preexisting board; output `board 6 41 0 21 47 30 12 319 0 0 0 0` | Outcome 1; seven comparable board/player fields, `BEFORE` board-name bytes, and all four cleared neighbor links match. Private LibZoo `BoardLen` is excluded from equality: its internal buffer length is 321, exactly two bytes above Pascal's 319 because LibZoo includes private metadata. |
| One-byte header | `SHORTHD.BRD`, byte `03` | Reopens the preexisting board; output `board 6 41 0 21 47 30 12 259 0 0 0 0` | Outcome 1; eight fields, board-name bytes, and all four neighbors match. The partial word changes Pascal's prior `$013F` to `$0103` (259). |
| Payload, 0 bytes present | `PAYZERO.BRD`, declared length 4 | `BoardCreate` state: `board 0 255 0 4 31 30 12 0 0 0 0 0` | Outcome 2; all eight selected board/player fields match. |
| Payload, 1 byte present | `PAYONE.BRD`, declared length 4 | Same `BoardCreate` state | Outcome 2; all eight selected board/player fields match. |
| Payload, 2 bytes present | `SHORTPAY.BRD`, declared length 4 | Same `BoardCreate` state | Outcome 2; all eight selected board/player fields match. |
| Payload, 3 bytes present | `PAYTHREE.BRD`, declared length 4 | Same `BoardCreate` state | Outcome 2; all eight selected board/player fields match. |

The production transfer comparison checks engine state and outcome. It does not compare the discarded Pascal payload's unread allocation suffix. Generated fixture SHA-256 values: `EMPTY.BRD` empty (`e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`); `SHORTHD.BRD` `084fed08b978af4d7d196a7446a86b58009e636b611db16211b65a9aadff29c5`; `PAYZERO.BRD` `c0ba8a33ac67f44abff5984dfbb6f56c46b880ac2b86e1f23e7fa9c402c53ae7`; `PAYONE.BRD` `2bd56058e48142aaa68d15b90df075232e441e78bb3eff596edf8f17d9dff7b5`; `SHORTPAY.BRD` `9fc1555c096242a12784fcc2a13dfbe307afa71c9bf98a4b1a46bf70fdb7aab1`; `PAYTHREE.BRD` `4ed9d0100f8db72ad536c09b6176abc232dd59a1bd7a06438801416a604cbcdc`.

The six Pascal editor-transfer output files are also recorded from the completed clean run: `BRDHDR.OUT` is 38 bytes with SHA-256 `5aa640be916c4a7f2c4e32dda24cf081902743ee85ca0ce1a219e959e00786f5`; `EMPTY.OUT` is 38 bytes with SHA-256 `d60e53c5e34d0914e655222f828165e7d954d8f1d02a9f159f9743ed8c225eca`; `BRDXFER.OUT`, `PAYZERO.OUT`, `PAYONE.OUT`, and `PAYTHREE.OUT` each contain the same 36-byte `BoardCreate` result and have SHA-256 `f88b2af33496a6e85a415bf9103252780d8e3f4a697af7d537ecfeedb3426b4a`. The raw `BRDIOPRB.OUT` is 294 bytes with SHA-256 `4a1f7296357c1c727cc8116a4ae49601f486a37d3b8a63abb77a294aa5601890`.

The clean verifier ran this comparator in its existing configured invocation and completed with exit code 0: 149 direct Pascal sessions, 74/74 configured Pascal-to-production comparisons, and F01–F10 (238 checkpoints / 824,821 state fields / 169 logical cycles). The added transfer cases do not add acceptance IDs or fixture-corpus completion credit. The serializer harness separately confirms production rejection of empty/partial record headers and declared payloads truncated at zero through three bytes (`ZOO_ERROR_INVAL`); this is production-only evidence for that parser API.

## Remaining E2-02 boundary work

This closes only these observed short-read rows. The broader frozen `.ZZT`, `.SAV`, `.BRD`, and `.HI` field/error matrix remains mandatory. Other allocator histories, complete comparison of any retained short-read buffer under a controlled corresponding production setup, and the remaining malformed input boundaries remain unverified. Preserve the observed unread-byte rule, but do not treat `$A5` as a universal file-format value: arbitrary uninitialized Pascal bytes are environment-dependent under the checklist's nondeterminism contract.
