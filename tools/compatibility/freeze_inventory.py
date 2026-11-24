#!/usr/bin/env python3
"""Freeze the Engine 2.0 checklist and source acceptance inventory once.

This intentionally refuses to overwrite existing frozen files. Review any
material compatibility defect and checklist change before creating a new
inventory revision.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from collections import defaultdict
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CHECKLIST = ROOT / "ENGINE_2.0_CHECKLIST.md"
OUT = ROOT / "tools/compatibility/acceptance_inventory.json"
HASH_OUT = ROOT / "tools/compatibility/reference_source_hashes.sha256"
SOURCE_UNITS = [
    "ZZT.PAS", "GAMEVARS.PAS", "GAME.PAS", "ELEMENTS.PAS", "OOP.PAS",
    "EDITOR.PAS", "INPUT.PAS", "KEYS.PAS", "SOUNDS.PAS", "TXTWIND.PAS",
    "VIDEO.PAS",
]
ROUTINE_RE = re.compile(r"^\s*(?:procedure|function)\s+([A-Za-z][A-Za-z0-9_]*)", re.I)
RECORD_RE = re.compile(r"(?ims)^\s*(\w+)\s*=\s*record\b(.*?)^\s*end\s*;")
FIELD_RE = re.compile(r"^\s*([A-Za-z][\w]*(?:\s*,\s*[A-Za-z][\w]*)*)\s*:\s*([^;]+);", re.I)
OOP_WORD_RE = re.compile(r"\bOopWord\s*=\s*'([^']+)'", re.I)
RANDOM_RE = re.compile(r"\bRandom\s*\(", re.I)
BRANCH_RE = re.compile(r"\b(if|case|while|repeat|for)\b", re.I)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def blocker_for(unit: str) -> str:
    if unit == "OOP.PAS":
        return "E2-05"
    if unit == "ELEMENTS.PAS":
        return "E2-04"
    if unit == "EDITOR.PAS":
        return "E2-09"
    if unit in ("INPUT.PAS", "KEYS.PAS"):
        return "E2-07"
    if unit == "SOUNDS.PAS":
        return "E2-08"
    if unit == "TXTWIND.PAS":
        return "E2-06"
    if unit == "GAMEVARS.PAS":
        return "E2-02"
    if unit == "GAME.PAS":
        return "E2-03"
    if unit == "VIDEO.PAS":
        return "E2-08"
    return "E2-01"


def parse_checklist(text: str) -> list[dict]:
    cases: list[dict] = []
    current: str | None = None
    counts: defaultdict[str, int] = defaultdict(int)
    for line_no, line in enumerate(text.splitlines(), 1):
        heading = re.match(r"^###\s+(E2-\d\d)\b", line)
        if heading:
            current = heading.group(1)
            continue
        if line.startswith("## "):
            current = None
        if current is None:
            continue
        m = re.match(r"^\s*-\s+\[([ x~])\]\s+(.*)$", line)
        gate = re.match(r"^\s*-\s+\*\*Pass:\*\*\s+(.*)$", line)
        if not m and not gate:
            continue
        counts[current] += 1
        if m:
            mark, body = m.groups()
            status = {"x": "complete", "~": "partial", " ": "pending"}[mark]
            evidence = "S" if re.search(r"source[- ]model|source-derived", body, re.I) else None
            if status == "complete" and evidence is None:
                evidence = "DP"
            if status == "partial":
                evidence = "DP"
        else:
            body = gate.group(1)
            status = "pending"
            evidence = None
        cases.append({
            "id": f"{current}-AC{counts[current]:03d}",
            "blocker": current,
            "kind": "acceptance_requirement",
            "requirement": body,
            "checklist_location": f"ENGINE_2.0_CHECKLIST.md:{line_no}",
            "status": status,
            "evidence_class": evidence,
            "production_implementation": [],
            "test_fixture": [],
            "oracle_output": None,
            "verification_result": "reported complete" if status == "complete" else ("partial evidence" if status == "partial" else "pending"),
        })
    return cases


def freeze() -> None:
    for output in (OUT, HASH_OUT):
        if output.exists():
            raise SystemExit(f"Refusing to overwrite frozen file: {output.relative_to(ROOT)}")
    source_root = ROOT / "third_party/reconstruction-of-zzt/SRC"
    files = [(unit, source_root / unit) for unit in SOURCE_UNITS]
    absent = [str(path.relative_to(ROOT)) for _, path in files if not path.is_file()]
    if absent:
        raise SystemExit("Missing reference source files: " + ", ".join(absent))

    routines: list[dict] = []
    branches: list[dict] = []
    rng_sites: list[dict] = []
    commands: dict[str, list[dict]] = defaultdict(list)
    alias_groups: list[dict] = []
    callbacks: list[dict] = []
    serialized_fields: list[dict] = []
    editor_mutations: list[dict] = []
    input_paths: list[dict] = []
    hashes: list[str] = []

    for unit, path in files:
        rel = path.relative_to(ROOT).as_posix()
        digest = sha256(path)
        hashes.append(f"{digest}  {rel}")
        lines = path.read_text(encoding="latin-1").splitlines()
        routine_lines: dict[str, list[int]] = defaultdict(list)
        current = "<unit>"
        for n, line in enumerate(lines, 1):
            rm = ROUTINE_RE.match(line)
            if rm:
                current = rm.group(1)
                routine_lines[current].append(n)
            if BRANCH_RE.search(line):
                kinds = [x.lower() for x in BRANCH_RE.findall(line)]
                branches.append({
                    "id": f"{blocker_for(unit)}-BR-{unit[:-4]}-{n:04d}",
                    "blocker": "E2-04" if unit == "ELEMENTS.PAS" else ("E2-05" if unit == "OOP.PAS" else blocker_for(unit)),
                    "kind": "pascal_branch_site",
                    "branch_ops": kinds,
                    "routine": current,
                    "source_location": f"{rel}:{n}",
                    "source_text": line.strip(),
                    "evidence_class": None,
                    "test_fixture": [],
                    "verification_result": "unclassified",
                })
            if RANDOM_RE.search(line):
                rng_sites.append({
                    "id": f"E2-07-RNG-{unit[:-4]}-{n:04d}",
                    "blocker": "E2-07" if unit != "SOUNDS.PAS" else "E2-08",
                    "kind": "pascal_random_call_site",
                    "routine": current,
                    "source_location": f"{rel}:{n}",
                    "source_text": line.strip(),
                    "evidence_class": None,
                    "production_implementation": [],
                    "test_fixture": [],
                    "verification_result": "unclassified",
                })
            line_words = OOP_WORD_RE.findall(line)
            if unit == "OOP.PAS" and len(line_words) > 1:
                alias_groups.append({
                    "id": f"E2-05-OOP-ALIAS-{n:04d}",
                    "blocker": "E2-05",
                    "kind": "oop_alias_group",
                    "words": [word.upper() for word in line_words],
                    "source_location": f"{rel}:{n}",
                    "routine": current,
                    "source_text": line.strip(),
                    "evidence_class": None,
                    "test_fixture": [],
                    "verification_result": "unclassified",
                })
            for word in line_words:
                entry = {"word": word.upper(), "source_location": f"{rel}:{n}", "routine": current, "source_text": line.strip()}
                commands[word.upper()].append(entry)

        for name, declarations in sorted(routine_lines.items()):
            case = {
                "id": f"{blocker_for(unit)}-ROUTINE-{unit[:-4]}-{name.upper()}",
                "blocker": blocker_for(unit),
                "kind": "source_routine",
                "symbol": name,
                "source_location": [f"{rel}:{n}" for n in declarations],
                "production_implementation": [],
                "test_fixture": [],
                "oracle_output": None,
                "evidence_class": None,
                "verification_result": "unclassified",
                "scope_status": "requires_shared_mapping_or_adapter_exclusion",
            }
            routines.append(case)
            if unit == "EDITOR.PAS":
                editor_mutations.append({**case, "kind": "editor_routine_scope_case", "verification_result": "requires_mutation_or_ui_scope_review"})
            if unit in ("INPUT.PAS", "KEYS.PAS"):
                input_paths.append({**case, "kind": "input_path_scope_case", "verification_result": "requires_core_or_adapter_scope_review"})

        if unit == "ELEMENTS.PAS":
            callback_names = [name for name in routine_lines if re.search(r"(Tick|Touch|Draw)", name, re.I)]
            for name in sorted(callback_names):
                callbacks.append({
                    "id": f"E2-04-CALLBACK-{name.upper()}",
                    "blocker": "E2-04",
                    "kind": "element_callback_routine",
                    "symbol": name,
                    "source_location": [f"{rel}:{n}" for n in routine_lines[name]],
                    "evidence_class": None,
                    "test_fixture": [],
                    "verification_result": "unclassified",
                })

        if unit == "GAMEVARS.PAS":
            content = "\n".join(lines)
            for rec in RECORD_RE.finditer(content):
                recname, body = rec.groups()
                start_line = content[:rec.start()].count("\n") + 1
                for offset, row in enumerate(body.splitlines(), 1):
                    fm = FIELD_RE.match(row)
                    if not fm:
                        continue
                    names, type_text = fm.groups()
                    for field in [x.strip() for x in names.split(",")]:
                        serialized_fields.append({
                            "id": f"{('E2-06' if recname == 'TStat' else 'E2-02')}-FIELD-{recname.upper()}-{field.upper()}",
                            "blocker": "E2-06" if recname == "TStat" else "E2-02",
                            "kind": "pascal_record_field",
                            "record": recname,
                            "field": field,
                            "type": type_text.strip(),
                            "source_location": f"{rel}:{start_line + offset}",
                            "evidence_class": None,
                            "test_fixture": [],
                            "verification_result": "unclassified",
                        })

    routine_set = {(x["blocker"], x["symbol"].upper()) for x in routines}
    command_cases = []
    for word, locations in sorted(commands.items()):
        command_cases.append({
            "id": f"E2-05-OOP-WORD-{word}",
            "blocker": "E2-05",
            "kind": "oop_command_condition_direction_or_tile_word",
            "word": word,
            "source_occurrences": locations,
            "evidence_class": None,
            "test_fixture": [],
            "verification_result": "unclassified",
        })

    acceptance = parse_checklist(CHECKLIST.read_text(encoding="utf-8"))
    inventory = {
        "schema_version": 1,
        "inventory_revision": "engine-2.0-initial-freeze",
        "frozen_date": date.today().isoformat(),
        "baseline": {
            "project_version_1_0": "LibZoo",
            "libzoo_commit": "7fb3e89a8467e66a4b8cbc290c708b9fa5290886",
            "reconstruction_repository": "asiekierka/reconstruction-of-zzt",
            "pascal_units": SOURCE_UNITS,
        },
        "evidence_classes": ["D", "S", "P", "DP", "I"],
        "acceptance_cases": acceptance,
        "source_inventory": {
            "routines": routines,
            "element_callback_routines": callbacks,
            "pascal_branch_sites": branches,
            "oop_words_aliases_and_parser_tokens": command_cases,
            "oop_alias_groups": alias_groups,
            "serialized_fields": serialized_fields,
            "rng_call_sites": rng_sites,
            "editor_routines_requiring_scope_or_mutation_mapping": editor_mutations,
            "input_paths_requiring_core_or_adapter_mapping": input_paths,
        },
        "summary": {
            "acceptance_cases": len(acceptance),
            "routines": len(routines),
            "element_callbacks": len(callbacks),
            "branch_sites": len(branches),
            "oop_words": len(command_cases),
            "oop_alias_groups": len(alias_groups),
            "serialized_fields": len(serialized_fields),
            "rng_call_sites": len(rng_sites),
            "editor_routines": len(editor_mutations),
            "input_paths": len(input_paths),
            "all_source_inventory_entries_initially_unclassified": True,
        },
        "change_policy": "Frozen inventory. Regeneration is a reviewed compatibility-scope change; do not overwrite this artifact automatically.",
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(inventory, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    HASH_OUT.write_text("\n".join(hashes) + "\n", encoding="ascii")
    print(json.dumps(inventory["summary"], indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--freeze", action="store_true", help="create the immutable initial inventory if absent")
    args = parser.parse_args()
    if not args.freeze:
        parser.error("pass --freeze to create the initial frozen inventory")
    freeze()
