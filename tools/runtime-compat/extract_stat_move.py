from pathlib import Path
import sys

source = Path(sys.argv[1]).read_text(encoding="utf-8")
name = "void zoo_stat_move("
start = source.index(name)
open_brace = source.index("{", start)
depth = 0
end = None
for index in range(open_brace, len(source)):
    if source[index] == "{":
        depth += 1
    elif source[index] == "}":
        depth -= 1
        if depth == 0:
            end = index + 1
            break
if end is None:
    raise SystemExit("Could not find the end of zoo_stat_move.")
Path(sys.argv[2]).write_text(source[start:end] + "\n", encoding="utf-8")
