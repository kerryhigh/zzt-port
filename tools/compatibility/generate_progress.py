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
SERIALIZATION_MANIFEST = ROOT / "tools/compatibility/serialization_fixture_manifest.json"
INTEGRATED_FIXTURE_MANIFEST = ROOT / "tools/compatibility/integrated_replay_fixture_manifest.json"
OUT = ROOT / "COMPATIBILITY_PROGRESS.md"


def main() -> None:
    inv = json.loads(INVENTORY.read_text(encoding="utf-8"))
    run = json.loads(RUN.read_text(encoding="utf-8")) if RUN.exists() else {}
    serialization_manifest = (
        json.loads(SERIALIZATION_MANIFEST.read_text(encoding="utf-8"))
        if SERIALIZATION_MANIFEST.exists()
        else {}
    )
    integrated_fixture_manifest = (
        json.loads(INTEGRATED_FIXTURE_MANIFEST.read_text(encoding="utf-8"))
        if INTEGRATED_FIXTURE_MANIFEST.exists()
        else {"required_fixture_count": 0, "fixtures": []}
    )
    integrated_fixtures = integrated_fixture_manifest.get("fixtures", [])
    required_fixture_count = integrated_fixture_manifest.get("required_fixture_count", 0)
    fixture_ids = [item.get("id") for item in integrated_fixtures]
    if len(integrated_fixtures) != required_fixture_count or len(set(fixture_ids)) != len(fixture_ids):
        raise ValueError("The frozen integrated replay manifest must contain exactly its declared number of unique fixture IDs")

    def fixture_has_completion_evidence(item: dict) -> bool:
        result = item.get("verification", {})
        return (
            item.get("status") == "complete"
            and {"D", "DP", "I"}.issubset(set(item.get("evidence_classes", [])))
            and result.get("result") == "passed"
            and result.get("clean_runner_exit_code") == 0
            and result.get("state_field_differences") == 0
            and result.get("cell_write_differences") == 0
            and result.get("save_round_trip", {}).get("stable_serialized_byte_differences") == 0
        )

    completed_integrated_fixtures = [
        item for item in integrated_fixtures if fixture_has_completion_evidence(item)
    ]
    pending_integrated_fixtures = [
        item for item in integrated_fixtures if item not in completed_integrated_fixtures
    ]
    serialization_formats = serialization_manifest.get("formats", [])
    serialization_samples_matched = sum(
        1
        for item in serialization_formats
        if item.get("comparison", {}).get("exit_code") == 0
        and item.get("comparison", {}).get("bytes_matched")
        == item.get("comparison", {}).get("bytes_total")
    )
    by_blocker: defaultdict[str, list[dict]] = defaultdict(list)
    acceptance_counts = Counter(item["status"] for item in inv["acceptance_cases"])
    unclassified_acceptance = [item for item in inv["acceptance_cases"] if not item.get("evidence_class")]
    evidence_class_names = set(inv["evidence_classes"])
    required_class_counts: Counter[str] = Counter()
    unmapped_required_classes = []
    for item in inv["acceptance_cases"]:
        by_blocker[item["blocker"]].append(item)
        required = item.get("required_evidence_classes", [])
        if not required or not set(required).issubset(evidence_class_names):
            unmapped_required_classes.append(item["id"])
        required_class_counts.update(required)
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
    latest_integrated = runner.get("latest_attempt", {}).get("integrated_replay_summary", {})
    integrated_diagnostic = integrated.get("diagnostic_negative_control", {})
    integrated_save = integrated.get("generated_save_comparison", {})
    integrated_roundtrip = integrated.get("generated_roundtrip_comparison", {})
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
    pascal_boardclose = runner.get("latest_attempt", {}).get("pascal_boardclose_brd", {})
    pascal_boardclose_worldsave = runner.get("latest_attempt", {}).get("pascal_boardclose_worldsave", {})

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
        f"- Required evidence classes mapped: **{len(inv['acceptance_cases']) - len(unmapped_required_classes)}/{len(inv['acceptance_cases'])}**; D: {required_class_counts['D']}, S: {required_class_counts['S']}, P: {required_class_counts['P']}, DP: {required_class_counts['DP']}, I: {required_class_counts['I']} cases (categories can overlap; required evidence is not a passing result).",
        f"- Acceptance cases missing or invalid required evidence classes: **{len(unmapped_required_classes)}**; IDs: {', '.join(unmapped_required_classes) or 'none'}.",
        f"- Acceptance cases without an evidence classification: **{len(unclassified_acceptance)}**; IDs: {', '.join(item['id'] for item in unclassified_acceptance) or 'none'}.",
        f"- Unclassified source inventory entries: **{unclassified}**; explicitly excluded adapter/presentation entries: **{explicitly_excluded}**.",
        "- Evidence totals remain separate: D counts compiled-Pascal sessions; DP counts configured direct comparisons; S counts source-derived model rows; P remains unenumerated; I counts only completed full integrated fixtures.",
        "",
        "## Evidence inventory",
        "",
        f"- Direct compiled-Pascal probe sessions in the last completed full suite: **{probes.get('sessions', 'not recorded')}** ({probes.get('standard_programs', 'not recorded')} standard programs; {probes.get('standalone_sessions', 'not recorded')} standalone sessions).",
        f"- Direct Pascal-to-production differential invocations in the last completed full suite: **{differential.get('passed', 'not recorded')}/{differential.get('configured', len(compare_calls))}** passed; failed: **{differential.get('failed', 'not recorded')}**; last complete result: **{differential.get('result', runner.get('pascal_suite_result', 'not recorded'))}**.",
        f"- Direct `.HI` Pascal-to-shared-engine byte comparisons: **{high_score.get('passed', 'not recorded')}/{high_score.get('configured', 'not recorded')}** passed ({high_score.get('save_cases', 'not recorded')} save, {high_score.get('load_reset_cases', 'not recorded')} load/reset, {high_score.get('add_cases', 'not recorded')} insertion cases); source-model rows: **{high_score.get('source_derived_model_rows', 'not recorded')}**.",
        f"- Direct Pascal `.BRD` `BoardClose` record: **{pascal_boardclose.get('record_bytes_matched', 'not recorded')}/{pascal_boardclose.get('record_size_bytes', 'not recorded')}** payload bytes matched through shared LibZoo import/export; external file: {pascal_boardclose.get('fixture_size_bytes', 'not recorded')} bytes; SHA-256 `{pascal_boardclose.get('sha256', 'not recorded')}`.",
        f"- Direct Pascal `BoardClose`/`WorldSave` differential: **{pascal_boardclose_worldsave.get('bytes_matched', 'not recorded')}/{pascal_boardclose_worldsave.get('world_size_bytes', 'not recorded')}** full `.ZZT` bytes matched production serialization; SHA-256 `{pascal_boardclose_worldsave.get('sha256', 'not recorded')}`; unexplained mismatches: {pascal_boardclose_worldsave.get('byte_differences', 'not recorded')}.",
        f"- Source-produced legacy-format samples in [`serialization_fixture_manifest.json`](tools/compatibility/serialization_fixture_manifest.json): **{serialization_samples_matched}/{len(serialization_formats)}** have complete byte comparisons; this establishes one valid sample per listed format, not the outstanding serialized-field and malformed-boundary matrix.",
        *([
            f"  - `{item.get('format', 'unknown')}`: {item.get('comparison', {}).get('bytes_matched', 'not recorded')}/{item.get('comparison', {}).get('bytes_total', 'not recorded')} bytes; SHA-256 `{item.get('sha256', 'not recorded')}`; {'pass' if item.get('comparison', {}).get('exit_code') == 0 and item.get('comparison', {}).get('bytes_matched') == item.get('comparison', {}).get('bytes_total') else 'pending/fail'} ({item.get('evidence_class', 'unclassified')})."
            for item in serialization_formats
        ] if serialization_formats else ["  - No source-produced serialization samples are recorded in a manifest."]),
        f"- Source-derived expectation cases: **{evidence.get('source_derived_expectations', 'not enumerated')}**; these are separate from compiled-Pascal probes.",
        f"- Production-only regression cases: **{evidence.get('production_only_regressions', 'not enumerated')}**; the finite case inventory is still being mapped.",
        f"- Integrated replay fixtures: **{len(completed_integrated_fixtures)}/{required_fixture_count}** complete from [`integrated_replay_fixture_manifest.json`](tools/compatibility/integrated_replay_fixture_manifest.json). Prototype comparisons: {integrated.get('prototype_field_comparisons', 'not recorded')} fields across {integrated.get('prototype_checkpoint_count', 'not recorded')} checkpoints; per-cycle checkpoints: {'present' if integrated.get('per_cycle_checkpoints') else 'not implemented'}.",
        f"- Integrated generated save: **{integrated_save.get('deterministic_bytes_matched', 'not recorded')}/{integrated_save.get('deterministic_bytes_total', 'not recorded')}** deterministic byte positions match; {integrated_save.get('classified_nondeterministic_positions', 'not recorded')} positions are classified as uninitialized Pascal padding or process-specific pointers; differing classified values: {integrated_save.get('classified_byte_differences', 'not recorded')}; production result: {integrated_save.get('production_result', 'not recorded')}.",
        f"- Integrated save reload: compiled Pascal WorldLoad={integrated_roundtrip.get('pascal_world_load_result', 'not recorded')}; production zoo_world_load={integrated_roundtrip.get('production_world_load_result', 'not recorded')}; round-trip save preserved {integrated_roundtrip.get('preserved_bytes_matched', 'not recorded')}/{integrated_roundtrip.get('preserved_bytes_total', 'not recorded')} bytes; {integrated_roundtrip.get('classified_process_pointer_positions', 'not recorded')} process-pointer positions classified.",
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
        f"- Required fixtures: {required_fixture_count} (fixed by `{integrated_fixture_manifest.get('schema_revision', 'unversioned')}`).",
        f"- Completed fixtures: {len(completed_integrated_fixtures)}/{required_fixture_count}; completed IDs: {', '.join(item['id'] for item in completed_integrated_fixtures) or 'none'}; pending IDs: {', '.join(item['id'] for item in pending_integrated_fixtures) or 'none'}.",
        f"- Prototype: {integrated.get('prototype_name', 'not recorded')} with {integrated.get('prototype_field_comparisons', 'not recorded')} total field comparisons across {integrated.get('prototype_checkpoint_count', 'not recorded')} checkpoints; per-checkpoint counts: {integrated.get('prototype_fields_compared', 'not recorded')}; this is not counted as a complete E2-10 fixture.",
        f"- Logical sound-queue events in the prototype: {integrated.get('sound_events_compared', 'not recorded')} events / {integrated.get('sound_event_fields_compared', 'not recorded')} event fields; checkpoint sound state: {'compared' if integrated.get('sound_checkpoint_state_compared') else 'outstanding'}; overflow: {integrated.get('sound_event_overflow', 'not recorded')}.",
        f"- Ordered playfield cell writes in the prototype: {integrated.get('cell_writes_compared', 'not recorded')} events / {integrated.get('cell_write_fields_compared', 'not recorded')} event fields; capture: {'matched' if integrated.get('logical_cell_write_capture') and not integrated.get('cell_event_overflow') else 'outstanding or overflowed'}; generated oracle sidecar SHA-256: {integrated.get('cell_oracle_output_sha256', 'not recorded')}.",
        f"- Generated `.SAV` comparison: {integrated_save.get('deterministic_bytes_matched', 'not recorded')}/{integrated_save.get('deterministic_bytes_total', 'not recorded')} stable bytes matched; {integrated_save.get('classified_nondeterministic_positions', 'not recorded')} classified positions ({integrated_save.get('classified_byte_differences', 'not recorded')} differing values); raw file size/hash: {integrated_save.get('size_bytes', 'not recorded')} bytes / `{integrated_save.get('sha256', 'not recorded')}`. Pascal save error result: {'captured' if integrated_save.get('pascal_error_outcome_compared') else 'no return value; failure-path outcome remains outstanding'}.",
        f"- Same-file reload/round-trip: Pascal WorldLoad={integrated_roundtrip.get('pascal_world_load_result', 'not recorded')}, production zoo_world_load={integrated_roundtrip.get('production_world_load_result', 'not recorded')}; {integrated_roundtrip.get('preserved_bytes_matched', 'not recorded')}/{integrated_roundtrip.get('preserved_bytes_total', 'not recorded')} loaded-file bytes preserved exactly; {integrated_roundtrip.get('classified_process_pointer_positions', 'not recorded')} process pointer bytes classified; input and second-save hashes: `{integrated_roundtrip.get('input_sha256', 'not recorded')}` / `{integrated_roundtrip.get('roundtrip_sha256', 'not recorded')}`.",
        f"- Per-cycle checkpoint comparisons: {'implemented' if integrated.get('per_cycle_checkpoints') else 'outstanding'}.",
        f"- State-transition checkpoints: {'implemented' if integrated.get('state_transition_checkpoints') else 'outstanding'}.",
        f"- First-divergence reporting: {'implemented' if integrated.get('first_divergence_reporting') else 'outstanding'}.",
        f"- First-divergence diagnostic negative control: {integrated_diagnostic.get('result', 'not recorded')}; {integrated_diagnostic.get('report', 'no controlled mismatch report recorded')}. This instrumentation check receives no replay-fixture or compatibility-case completion credit.",
        *([
            f"- `{item['id']}` {item['scenario']}: **{item.get('verification', {}).get('result', 'unclassified')}**; {item.get('verification', {}).get('checkpoint_count', 'not recorded')} checkpoints / {item.get('verification', {}).get('state_field_comparisons', 'not recorded')} state fields; {item.get('verification', {}).get('state_field_differences', 'not recorded')} state differences; {item.get('verification', {}).get('logical_sound_events', 'not recorded')} sound events; {item.get('verification', {}).get('ordered_playfield_cell_writes', 'not recorded')} cell writes; {item.get('verification', {}).get('save_round_trip', {}).get('stable_serialized_bytes_compared', 'not recorded')} stable save bytes compared with {item.get('verification', {}).get('save_round_trip', {}).get('stable_serialized_byte_differences', 'not recorded')} differences."
            for item in integrated_fixtures
        ] if integrated_fixtures else ["- No frozen integrated fixture inventory is recorded."]),
        *([
            f"- {item.get('fixture', 'unnamed replay')}: {item.get('checkpoints', 'not recorded')} checkpoints; {item.get('field_comparisons', 'not recorded')} state-field comparisons total; {item.get('result', 'unclassified')} ({item.get('evidence_class', 'unclassified')} evidence)."
            for item in integrated.get("per_cycle_results", [])
        ] if integrated.get("per_cycle_results") else ["- Current per-cycle results: none recorded."]),
        *([
            f"  - {checkpoint.get('checkpoint', 'unnamed checkpoint')}: {checkpoint.get('fields_compared', 'not recorded')} fields; {checkpoint.get('result', 'unclassified')}."
            for item in integrated.get("per_cycle_results", [])
            for checkpoint in item.get("checkpoint_results", [])
        ] or ["- Per-checkpoint results: not recorded."]),
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
        f"- Latest full Pascal-runner attempt: exit code {runner.get('latest_attempt', {}).get('full_pascal_runner_exit_code', 'not recorded')}; {runner.get('latest_attempt', {}).get('status', 'not recorded')}"
        + (f". {runner['latest_attempt']['failure']}" if runner.get('latest_attempt', {}).get('failure') else "."),
        f"- Latest integrated Pascal differential `{latest_integrated.get('fixture_id', 'not recorded')}`: exit {runner.get('latest_attempt', {}).get('integrated_differential_exit_code', 'not recorded')}; {latest_integrated.get('state_fields_compared', 'not recorded')} snapshot fields, {latest_integrated.get('sound_events', 'not recorded')} logical sound events, {latest_integrated.get('cell_writes', 'not recorded')} ordered playfield cell writes, {latest_integrated.get('generated_save_stable_bytes_matched', 'not recorded')}/{latest_integrated.get('generated_save_stable_bytes_total', 'not recorded')} generated-save stable bytes, and {latest_integrated.get('roundtrip_stable_bytes_matched', 'not recorded')}/{latest_integrated.get('roundtrip_stable_bytes_total', 'not recorded')} loaded-file round-trip stable bytes matched.",
        f"- Toolchain: {runner.get('toolchain', {}).get('pascal', 'not recorded')}; {runner.get('toolchain', {}).get('dosbox_x', 'DOSBox-X version not recorded')}.",
        f"- Source revision: LibZoo baseline `{inv['baseline']['libzoo_commit']}`; local LibZoo overlay `{runner.get('libzoo_local_commit', 'not recorded')}`; Pascal source hashes in the manifest; root Git revision: {runner.get('root_revision', 'not yet recorded')}.",
        f"- Last completed full Pascal suite exit code: {runner.get('pascal_exit_code', 'not recorded')}; command recorded: `{runner.get('command', 'not recorded')}`.",
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
