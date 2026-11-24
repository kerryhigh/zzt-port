from pathlib import Path
import sys

source = Path(sys.argv[1]).read_text(encoding="utf-8")


def extract(name):
    start = source.index(f"static void {name}(")
    open_brace = source.index("{", start)
    depth = 0
    for index in range(open_brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1] + "\n"
    raise SystemExit(f"Could not find the end of {name}.")


Path(sys.argv[2]).write_text(extract("zoo_e_transporter_move"), encoding="utf-8")
Path(sys.argv[3]).write_text(extract("zoo_e_transporter_touch"), encoding="utf-8")
