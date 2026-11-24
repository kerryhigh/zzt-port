from pathlib import Path
import sys


if len(sys.argv) != 3:
    raise SystemExit("Usage: extract_stat_add.py zoo_game.c output.inc")

source = Path(sys.argv[1]).read_text(encoding="utf-8")


def extract_function(signature: str) -> str:
    name_index = source.index(signature)
    start = source.rfind("\n", 0, name_index) + 1
    open_brace = source.index("{", name_index)
    depth = 0
    for index in range(open_brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise SystemExit(f"Could not find the end of {signature}.")


helpers = extract_function("void zoo_board_data_heap_mark_allocated(")
stat_add = extract_function("void zoo_stat_add(")
Path(sys.argv[2]).write_text(helpers + "\n\n" + stat_add + "\n", encoding="utf-8")
