from pathlib import Path
import sys

source = Path(sys.argv[1]).read_text(encoding="utf-8")


def extract_function(signature):
    name_index = source.index(signature)
    start = source.rfind("\n", 0, name_index) + 1
    if signature.startswith("Java_com_"):
        start = source.rfind("JNIEXPORT jint JNICALL", 0, name_index)
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


def extract_context_type():
    signature = "typedef struct editor_flood_fill_context {"
    start = source.index(signature)
    open_brace = source.index("{", start)
    depth = 0
    for index in range(open_brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                end = source.index(";", index) + 1
                return source[start:end]
    raise SystemExit("Could not find the editor flood-fill context type.")


Path(sys.argv[2]).write_text(
    "\n\n".join((
        extract_context_type(),
        extract_function("editor_flood_fill_apply("),
        extract_function("Java_com_kerry_zzt_ZztNative_editorFloodFill("),
    )) + "\n",
    encoding="utf-8",
)
