#!/usr/bin/env python3
"""Generate COMPATIBILITY_PROGRESS.md from frozen acceptance data and run record."""

from __future__ import annotations

import json
import re
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INVENTORY = ROOT / "tools/compatibility/acceptance_inventory.json"
RUN = ROOT / "tools/compatibility/latest_verification.json"
OUT = ROOT / "COMPATIBILITY_PROGRESS.md"


def main() -> None:
    inv = json.loads(INVENTORY.read_text(encoding="utf-8"))
    run = json.loads(RUN.read_text(encoding="utf-8")) if RUN.exists() else {}
    by_blocker: defaultdict[str, list[dict]] = defaultdict(list)
    acceptance_counts = Counter(item["status"] for item in inv["acceptance_cases"])
    for item in inv["acceptance_cases"]:
        by_blocker[item["blocker"]].append(item)
    closed_set = set(run.get("closed_blockers", []))
    closed = 0
    table = []
    for n in range(1, 12):
        key = f"E2-{n:02d}"
        items = by_blocker[key]
        counts = Counter(item["status"] for item in items)
        total = len(items)
        done = counts["complete"]
        percent = f"{100 * done / total:.1f}%" if total else "not established"
        if key in closed_set and total and counts["complete"] == total and counts["partial"] == 0 and counts["pending"] == 0:
            closed += 1
        # A frozen checklist marker is necessary, but final closure also needs its evidence gate.
        status = "closed" if key in closed_set and counts["complete"] == total else ("partial" if counts["complete"] or counts["partial"] else "open")
        table.append(f"| {key} | {status} | {done}/{total} | {counts['partial']} | {counts['pending']} | {percent} |")

    # Count direct production comparisons from the actual runner command list.
    verify_script = ROOT / "tools/pascal-compat/verify.ps1"
    script_text = verify_script.read_text(encoding="utf-8")
    compare_calls = re.findall(r"&\s*node\s+'(?:tools/[^']*/)?(compare-[^']+\.js)'", script_text)
    runner = run.get("verification", {})
    probes = runner.get("direct_pascal", {})
    differential = runner.get("direct_differential_comparisons", {})
    integrated = runner.get("integrated_replay", {})
    evidence = runner.get("evidence_case_counts", {})
    misc = runner.get("other_suites", {})
    source = inv["source_inventory"]
    source_entries = [item for group in source.values() for item in group]
    explicitly_excluded = sum(1 for item in source_entries if item.get("scope_status", "").startswith("excluded_"))
    unclassified = sum(
        1 for item in source_entries
        if not item.get("evidence_class") and not item.get("scope_status", "").startswith("excluded_")
    )
    high_score = runner.get("high_score_differential", {})

    known_mismatches = runner.get("known_unexplained_mismatches", [])
    discrepancies = runner.get("known_discrepancies", [])
    limitations = runner.get("non_blocking_limitations", [])
    remaining = runner.get("remaining_mandatory_work", [])

    lines = [
        "# Engine 2.0 compatibility progress",
        "",
        f"**Milestone status:** {'complete' if closed == 11 and not remaining else 'in progress'}  ",
        f"**Mandatory blockers closed:** {closed}/11  ",
        f"**Latest verification date:** {runner.get('date', 'not recorded')}  ",
        f"**Frozen inventory:** revision `{inv['inventory_revision']}` ({inv['frozen_date']})  ",
        f"**Reference source hashes:** [`reference_source_hashes.sha256`](tools/compatibility/reference_source_hashes.sha256)",
        "",
        "Counts below are generated from the frozen checklist inventory and the latest recorded verification run. Checklist completion markers do not close a release blocker until the blocker pass gate and clean rerun pass.",
        "",
        "## Blocker status",
        "",
        "| Blocker | Status | Complete / total acceptance cases | Partial | Pending | Checklist coverage |",
        "|---|---:|---:|---:|---:|---:|",
        *table,
        "",
        "A percentage is shown only for a finite checklist-case denominator. Source enumeration counts are reported separately and are not folded into these percentages.",
        "",
        "## Acceptance case status",
        "",
        f"- Passing/complete: **{acceptance_counts['complete']}**; partial: **{acceptance_counts['partial']}**; pending: **{acceptance_counts['pending']}**; failing: **{run.get('verification', {}).get('failing_acceptance_cases', 'not separately enumerated')}**.",
        f"- Unclassified source inventory entries: **{unclassified}**; explicitly excluded adapter/presentation entries: **{explicitly_excluded}**.",
        "- Evidence totals remain separate: D counts compiled-Pascal sessions; DP counts configured direct comparisons; S counts source-derived model rows; P remains unenumerated; I counts only completed full integrated fixtures.",
        "",
        "## Evidence inventory",
        "",
        f"- Direct compiled-Pascal probe sessions: **{probes.get('sessions', 'not recorded')}** ({probes.get('standard_programs', 'not recorded')} standard programs; {probes.get('standalone_sessions', 'not recorded')} standalone sessions).",
        f"- Direct Pascal-to-production differential comparisons: **{differential.get('passed', 'not recorded')}/{differential.get('configured', len(compare_calls))}** passed; failed: **{differential.get('failed', 'not recorded')}**; run result: **{differential.get('result', runner.get('pascal_suite_result', 'not recorded'))}**.",
        f"- Direct `.HI` Pascal-to-shared-engine byte comparisons: **{high_score.get('passed', 'not recorded')}/{high_score.get('configured', 'not recorded')}** passed ({high_score.get('save_cases', 'not recorded')} save, {high_score.get('load_reset_cases', 'not recorded')} load/reset, {high_score.get('add_cases', 'not recorded')} insertion cases); source-model rows: **{high_score.get('source_derived_model_rows', 'not recorded')}**.",
        f"- Source-derived expectation cases: **{evidence.get('source_derived_expectations', 'not enumerated')}**; these are separate from compiled-Pascal probes.",
        f"- Production-only regression cases: **{evidence.get('production_only_regressions', 'not enumerated')}**; the finite case inventory is still being mapped.",
        f"- Integrated replay fixtures: **{integrated.get('completed_fixtures', 0)}/{integrated.get('required_fixtures', 10)}** complete. Prototype comparisons: {integrated.get('prototype_field_comparisons', 'not recorded')} fields across {integrated.get('prototype_checkpoint_count', 'not recorded')} checkpoints; per-cycle checkpoints: {'present' if integrated.get('per_cycle_checkpoints') else 'not implemented'}.",
        f"- Other suite results: serializer={misc.get('serializer', 'not recorded')}; runtime={misc.get('runtime', 'not recorded')}; Android CP437/key bridge={misc.get('android_compat', 'not recorded')}; Android build={misc.get('android_build', 'not recorded')}; Windows CTest={misc.get('windows_ctest', 'not recorded')}.",
        "",
        "## Frozen source inventory",
        "",
        "| Inventory | Cases | Classified with D/S/P/DP/I | Explicitly excluded | Unclassified |",
        "|---|---:|---:|---:|---:|",
    ]
    for label, key in [
        ("Pascal routines", "routines"),
        ("Element callback routines", "element_callback_routines"),
        ("Pascal branch sites", "pascal_branch_sites"),
        ("OOP words and aliases", "oop_words_aliases_and_parser_tokens"),
        ("Serialized fields", "serialized_fields"),
        ("RNG call sites", "rng_call_sites"),
        ("Editor routines requiring scope/mutation mapping", "editor_routines_requiring_scope_or_mutation_mapping"),
        ("Input paths requiring core/adapter mapping", "input_paths_requiring_core_or_adapter_mapping"),
    ]:
        items = source[key]
        classified = sum(1 for item in items if item.get("evidence_class"))
        excluded = sum(1 for item in items if item.get("scope_status", "").startswith("excluded_"))
        lines.append(f"| {label} | {len(items)} | {classified} | {excluded} | {len(items)-classified-excluded} |")
    lines.extend([
        "",
        f"Total unclassified source inventory entries: **{unclassified}**; explicitly excluded: **{explicitly_excluded}**. Source locations, routine-to-production candidates, callback branches, parser words, serialized fields, RNG sites, editor procedures, and input routines are kept as explicit acceptance records; an empty test mapping is not treated as coverage.",
        "",
        "## Per-cycle integrated replay",
        "",
        f"- Required fixtures: {integrated.get('required_fixtures', 10)}.",
        f"- Completed fixtures: {integrated.get('completed_fixtures', 0)}.",
        f"- Prototype: {integrated.get('prototype_name', 'not recorded')} with {integrated.get('prototype_field_comparisons', 'not recorded')} total field comparisons across {integrated.get('prototype_checkpoint_count', 'not recorded')} checkpoints ({integrated.get('prototype_fields_compared', 'not recorded')} fields each); this is not counted as a complete E2-10 fixture.",
        f"- Per-cycle checkpoint comparisons: {'implemented' if integrated.get('per_cycle_checkpoints') else 'outstanding'}.",
        f"- State-transition checkpoints: {'implemented' if integrated.get('state_transition_checkpoints') else 'outstanding'}.",
        f"- First-divergence reporting: {'implemented' if integrated.get('first_divergence_reporting') else 'outstanding'}.",
        *([
            f"- {item.get('fixture', 'unnamed replay')}: {item.get('checkpoints', 'not recorded')} checkpoints × {item.get('fields_per_checkpoint', 'not recorded')} fields = {item.get('field_comparisons', 'not recorded')} comparisons; {item.get('result', 'unclassified')} ({item.get('evidence_class', 'unclassified')} evidence)."
            for item in integrated.get("per_cycle_results", [])
        ] if integrated.get("per_cycle_results") else ["- Current per-cycle results: none recorded."]),
        "",
        "## Mismatches, limitations, and remaining work",
        "",
        f"- Known unexplained mismatches in the latest recorded run: **{len(known_mismatches)}**. This count does not imply complete coverage; unexplained mismatch triage is still a release gate.",
    ])
    for item in known_mismatches:
        lines.append(f"  - {item}")
    lines.extend(["", "### Mandatory remaining work", ""])
    for item in remaining:
        lines.append(f"- {item}")
    if not remaining:
        lines.append("- See open E2-01 through E2-11 acceptance cases above; the checklist remains authoritative.")
    lines.extend(["", "### Non-blocking limitations and optional investigations", ""])
    for item in limitations:
        lines.append(f"- {item}")
    if not limitations:
        lines.append("- No additional limitations were enumerated in the latest run record. Environment-dependent behavior remains governed by the checklist nondeterminism table.")
    lines.extend([
        "",
        "## Known discrepancies and unresolved questions",
        "",
    ])
    for item in discrepancies:
        lines.append(f"- {item}")
    if not discrepancies:
        lines.append("- See the known-quirks table in `ENGINE_2.0_CHECKLIST.md`; no separate discrepancy ledger is recorded.")
    lines.extend([
        "",
        "## Latest verification record",
        "",
        f"- Toolchain: {runner.get('toolchain', {}).get('pascal', 'not recorded')}; {runner.get('toolchain', {}).get('dosbox_x', 'DOSBox-X version not recorded')}.",
        f"- Source revision: LibZoo baseline `{inv['baseline']['libzoo_commit']}`; local LibZoo overlay `{runner.get('libzoo_local_commit', 'not recorded')}`; Pascal source hashes in the manifest; root Git revision: {runner.get('root_revision', 'not yet recorded')}.",
        f"- Pascal suite exit code: {runner.get('pascal_exit_code', 'not recorded')}; command recorded: `{runner.get('command', 'not recorded')}`.",
        f"- Run record: [`latest_verification.json`](tools/compatibility/latest_verification.json).",
        "",
        "## Recent progress",
        "",
    ])
    for item in runner.get("recent_progress", []):
        lines.append(f"- {item}")
    if not runner.get("recent_progress"):
        lines.append("- Compatibility reporting and the frozen source/checklist inventory are being established; Engine 2.0 remains open.")
    lines.extend(["", "Generated by `python tools/compatibility/generate_progress.py`. Do not edit this report manually.", ""])
    OUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {OUT.relative_to(ROOT)}: {closed}/11 blockers closed; {len(inv['acceptance_cases'])} finite checklist acceptance cases.")


if __name__ == "__main__":
    main()
