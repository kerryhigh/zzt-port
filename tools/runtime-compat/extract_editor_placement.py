from pathlib import Path
import sys


if len(sys.argv) != 3:
    raise SystemExit("Usage: python extract_editor_placement.py native-lib.c output.inc")

source = Path(sys.argv[1]).read_text(encoding="utf-8")
signatures = (
    "static void editor_clipboard_clear(",
    "static uint8_t editor_element_color(",
    "static bool editor_prepare_tile(",
    "static void editor_draw_tile_and_neighbors(",
    "static int editor_place_element(",
    "static int editor_place_pattern_element(",
    "static int editor_paste_tile(",
    "JNIEXPORT jint JNICALL\nJava_com_kerry_zzt_ZztNative_editorCopyTile(",
)


def extract(signature):
    name = signature.split("(", 1)[0].split()[-1]
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
    raise SystemExit(f"Could not find the end of {name}.")


Path(sys.argv[2]).write_text(
    "\n\n".join(extract(signature) for signature in signatures) + "\n",
    encoding="utf-8",
)
