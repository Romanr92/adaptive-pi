#!/usr/bin/env python3
"""Evaluate Release 3 requirement evidence and promote eligible Approved entries."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[2]
REQUIREMENT_FILES = [
    REPO_ROOT / "docs/requirements/release-3-ara-core.md",
    REPO_ROOT / "docs/requirements/release-3-ara-log.md",
]


def _find_requirement_id(doc_text: str) -> str | None:
    match = re.search(r"^##\s+(AP-R3-(?:CORE|LOG)-\d{3})\b", doc_text, re.MULTILINE)
    if match:
        return match.group(1)
    return None


def _extract_status(doc_text: str) -> str | None:
    match = re.search(r"^- Status: (Approved|Implemented)\b", doc_text, re.MULTILINE)
    if match:
        return match.group(1)
    return None


def _update_status(doc_text: str, new_status: str) -> str:
    return re.sub(
        r"^- Status: (?:Approved|Implemented)\b",
        f"- Status: {new_status}",
        doc_text,
        count=1,
        flags=re.MULTILINE,
    )


def _insert_evidence(doc_text: str, evidence_link: str | None) -> str:
    if not evidence_link:
        return doc_text
    if re.search(r"^- Evidence: ", doc_text, re.MULTILINE):
        return doc_text
    return re.sub(
        r"^- Status: Implemented\b",
        f"- Status: Implemented\n- Evidence: {evidence_link}",
        doc_text,
        count=1,
        flags=re.MULTILINE,
    )


def _has_required_evidence(requirement_id: str, evidence: dict[str, Any]) -> bool:
    entry = evidence.get(requirement_id, {})
    if not isinstance(entry, dict):
        return False

    if entry.get("status") != "pass":
        return False

    required_checks = entry.get("required_checks") or []
    if not required_checks:
        return False

    return True


def evaluate_requirement(doc_text: str, evidence: dict[str, Any]) -> dict[str, Any]:
    requirement_id = _find_requirement_id(doc_text)
    current_status = _extract_status(doc_text)
    if requirement_id is None or current_status != "Approved":
        return {
            "requirement_id": requirement_id,
            "new_status": current_status or "Unknown",
            "updated_text": doc_text,
            "reason": "Requirement is not an Approved Release 3 entry.",
        }

    if _has_required_evidence(requirement_id, evidence):
        evidence_link = str(evidence[requirement_id].get("evidence_url", "")).strip()
        updated = _insert_evidence(_update_status(doc_text, "Implemented"), evidence_link)
        return {
            "requirement_id": requirement_id,
            "new_status": "Implemented",
            "updated_text": updated,
            "reason": "Pass criteria met for the mapped verification set and the evidence is traceable to the requirement ID.",
        }

    return {
        "requirement_id": requirement_id,
        "new_status": "Approved",
        "updated_text": doc_text,
        "reason": "No complete evidence set was available for this requirement; status remains Approved.",
    }


def _update_file_requirements(file_path: Path, evidence: dict[str, Any]) -> tuple[str, list[str]]:
    text = file_path.read_text(encoding="utf-8")
    changed_requirements: list[str] = []
    updated_text = text
    pattern = r"^##\s+(AP-R3-(?:CORE|LOG)-\d{3})\b.*?(?=^##\s|\Z)"

    for match in reversed(list(re.finditer(pattern, text, re.MULTILINE | re.DOTALL))):
        section = match.group(0)
        result = evaluate_requirement(section, evidence)
        if result["new_status"] == "Implemented" and section != result["updated_text"]:
            changed_requirements.append(result["requirement_id"])
            updated_text = (
                updated_text[: match.start()] +
                result["updated_text"] +
                updated_text[match.end() :]
            )

    return updated_text, changed_requirements


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dry-run", action="store_true", help="show which requirements would change without editing files")
    parser.add_argument("--evidence-file", type=Path, default=REPO_ROOT / "build" / "requirement-evidence.json")
    args = parser.parse_args()

    evidence: dict[str, Any] = {}
    if args.evidence_file.exists():
        try:
            evidence = json.loads(args.evidence_file.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            print(f"Ignoring unreadable evidence file: {args.evidence_file}", file=sys.stderr)
            evidence = {}

    changed_requirements: list[str] = []
    for file_path in REQUIREMENT_FILES:
        if not file_path.exists():
            continue
        updated_text, requirements = _update_file_requirements(file_path, evidence)
        changed_requirements.extend(requirements)

        if args.dry_run:
            if requirements:
                print(f"Would update {file_path} ({', '.join(requirements)})")
        else:
            if updated_text != file_path.read_text(encoding="utf-8"):
                file_path.write_text(updated_text, encoding="utf-8")

    if args.dry_run:
        if changed_requirements:
            print("Requirements eligible for promotion:")
            for requirement in sorted(set(changed_requirements)):
                print(f"- {requirement}")
        else:
            print("No requirements eligible for promotion.")
        return 0

    return 0


if __name__ == "__main__":
    sys.exit(main())
