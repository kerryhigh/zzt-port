# ZZT serializer compatibility checks

Run `powershell -ExecutionPolicy Bypass -File tools\serializer-compat\verify.ps1` from the project root. The script uses the NDK version pinned in `app/build.gradle` and requires Node.js.

It compiles the production `zoo_game_io.c`, `zoo_game.c`, `zoo_io.c`, and shared LibZoo `.BRD` codec into a temporary WebAssembly module, then checks:

- The bundled 26,465-byte `DEMO.ZZT` after loading, opening and closing the current board, and saving the world.
- The production `zoo_world_save` close/write/reopen path used by Android, confirming that play marks `.SAV` byte 264 as `1` and that the resulting save loads and resaves byte-for-byte.
- Black-key state encoded by the Pascal `Keys[0]` alias into the high byte of `Gems`, with all real `Keys[1..7]` bytes clear, survives byte-exact `.ZZT` serialization and the production `.SAV` save/load/resave path.
- Every external board record in the demo world.
- The shared production `.BRD` parser and exporter round-trip all 16 demo board records byte-for-byte.
- Loading an unmarked legacy header and saving it in the current marked format.
- Rejecting a short 512-byte header while matching Pascal's unchecked board-record reads: partial length words preserve unread destination bytes, short payloads are accepted, a missing trailing record is treated from the current destination length, and a stored `0xFFFF` length is allocated/read as the unsigned 65,535-byte `Word` argument Pascal passes to `GetMem` and `BlockRead`. The adapter leaves unread payload tails untouched in its allocated buffer, like Pascal `GetMem` plus `BlockRead`; those bytes depend on allocator state and are not a stable file-format value.
- Pascal's title-only load behavior: force board 0 and `IsSave`, load only the first board from a header declaring more boards, and accept the same prefix in a full-board load when the current board itself is present.
- Embedded-NUL Pascal strings, noncanonical Boolean bytes, reserved bytes, and `DataPos` values where `DataLen` is zero.
- Repeated Pascal short-string assignments to world names, board names, flags, and messages, including stale payload tails left by earlier longer assignments.
- The editor's unchanged-title comparison against the exposed raw Pascal bytes, including embedded NULs and length bytes larger than the field capacity.
- 256 deterministic world-save round-trips covering every possible Pascal short-string length byte in the world name, every flag slot, board name, and board message, with arbitrary CP437 bytes and embedded NULs.
- The production `WorldUnload`/`WorldCreate` reset path, including embedded-NUL world-name and flag payload bytes, plus `BoardCreate` clearing board-string lengths while retaining the fixed name and message payload tails.
- Pascal byte-underflow behavior for a zero-count RLE run and stopping at board cell 1,500 when a run has cells left.
- Direct Turbo Pascal `BoardOpen` traces now confirm both RLE decoder quirks against the production C reader.
- A direct Turbo Pascal sweep of all 256 RLE run-count bytes compares every decoded tile and the following board-info byte against the production C reader; count zero consumes 256 cells through byte underflow.
- Production world serialization matches `BoardClose`'s maximum RLE run split: a uniform 1,500-cell board emits five 255-cell runs and a final 225-cell run.
- The editor's production `.BRD` import/export path treats the two-byte record length as Pascal's unsigned `Word` read count: `0xFFFF` reads 65,535 bytes, board parsing ignores trailing payload padding, and export re-encodes the parsed board canonically. It also repeats a zero-count RLE run for 256 cells.
- The editor's `.BRD` importer rejects incomplete record headers and declared payloads truncated at 0, 1, 2, or 3 bytes. Direct Pascal probes report I/O error 100, show that an empty length word preserves its seeded destination and a one-byte word replaces only its low byte, and retain seeded values in the unread payload suffix. The six-case original EditorTransferBoard differential compares empty/one-byte headers and payload lengths 0–3 against the shared engine operation. The empty-header comparison excludes LibZoo's private +2-byte internal buffer metadata length; production state, board name, and four cleared neighbors match. The Pascal payload suffix is not compared to production because the incomplete payload is discarded there. The editor transfer path's `BoardCreate` result is shared engine behavior; stream-close failures that Pascal does not inspect remain adapter behavior.
- Shared stat text is written with a negative `DataLen` reference to the earlier stat and resolves to the same pointer and bytes when loaded.

The harness supplies a small allocator and the two stat lifecycle helpers needed by the isolated serializer path. It checks file-format behavior only; it does not establish complete gameplay equivalence with the Turbo Pascal runtime or physical Android behavior.
