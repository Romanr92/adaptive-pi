# Release 3 requirement status promotion

## Purpose

This workflow enforces the repository rule that a Release 3 requirement moves from
`Approved` to `Implemented` only when the implementation and verification evidence
are both present and traceable.

The project does not treat a Copilot review as verification. A human or agent may
help summarize evidence, but the promotion decision remains tied to actual test or
build results captured in the repository and CI logs.

## Rule used by the automation

The source-of-truth rule is defined in [docs/requirements/README.md](../requirements/README.md):

- the implementation must exist;
- the mapped checks must pass in the required configuration(s);
- the evidence must trace back to the requirement ID and relevant CI/test run;
- incomplete or ambiguous evidence keeps the requirement at `Approved`.

The automation therefore refuses promotion when the evidence file is missing,
ambiguous, or does not include the required checks for the candidate requirement.

## Workflow behavior

The workflow in [.github/workflows/release-3-requirement-status-promotion.yml](../../.github/workflows/release-3-requirement-status-promotion.yml)
can run:

- on demand with `workflow_dispatch`;
- after a successful `Host CI` run through `workflow_run`.

It reads a JSON evidence file, evaluates the Release 3 requirement entries, and
updates only the eligible `Approved` entries. When no evidence or no valid pass
set is present, the status remains `Approved` and the workflow exits without
changing the requirement file.

## Evidence format

The evidence file is a JSON mapping keyed by the requirement IDs such as
`AP-R3-CORE-006` and `AP-R3-LOG-003`.

Example:

```json
{
  "AP-R3-CORE-006": {
    "status": "pass",
    "required_checks": [
      "AP_R3_CORE_006_ResultCreationAndEmplacement",
      "AP_R3_CORE_006_ResultCreationAndEmplacement.DirectConstructionSelectsValueOrError"
    ],
    "evidence_url": "https://github.com/Romanr92/adaptive-pi/actions/runs/123456789"
  }
}
```

The script checks the `status` and the `required_checks` list before promoting a
requirement. It uses the evidence URL for traceability, but it never accepts a
Copilot-only assessment as proof.

## Permissions and safety

The workflow requests:

- `contents: write` to update the requirement markdown files when eligible;
- `pull-requests: write` to support branch-oriented PR-based workflows.

The workflow is scoped to the Release 3 requirement files only and preserves the
rest of the repository. It does not edit unrelated requirements or duplicate
status entries.

## Rerun and override behavior

- Re-run the workflow after a new CI run or after updating the evidence JSON.
- If evidence is incomplete or the required exception-enabled/host checks are not
  present, the requirement stays `Approved`.
- To override a mistaken promotion, restore the previous file version from Git and
  rerun the workflow with corrected evidence or a missing evidence file.
- The script supports a dry run for validation and review:

```bash
python3 scripts/requirements/promote_release3_requirements.py --dry-run \
  --evidence-file build/requirement-evidence.json
```

## Limitation

This repository does not currently expose a supported GitHub Copilot workflow that
can directly invoke a repository-specific Copilot judgment from CI. This
implementation therefore uses the closest supported workflow: repository evidence,
CI results, and a deterministic rule-based promotion check. It does not pretend to
perform a fully autonomous Copilot-mediated status promotion.
