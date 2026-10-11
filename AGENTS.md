# AdaptivePi agent instructions

AdaptivePi is an educational, clean-room Adaptive AUTOSAR-inspired project. It does not claim AUTOSAR conformance or use proprietary AUTOSAR implementation code.

## Source of truth

- Before any implementation, feature question, requirement discussion, or unit-test work, read `AGENTS.md`, `README.md`, `docs/requirements/README.md`, and the relevant skill file in `.agents/skills/`.
- Read the relevant requirement file before discussing or implementing a requirement.
- Treat agreed requirement text (or a legacy Approved baseline), its verification method, and its stated deviation as the implementation baseline.
- Consult `README.md` and `docs/` for information about the project contents, architecture, guides or requirements.
- Consult `docs/adr/`, relevant code, and tests for architectural and implementation context.
- Use conversation history when available, but do not let remembered decisions override the current repository. If they disagree, point out the difference.
- Do not add AUTOSAR features beyond the project's stated scope merely to match a specification.
- Use the requirement text and verification method as the baseline for implementation and tests



## Creating requirements

Every LLM or coding agent creating or revising an AUTOSAR-derived requirements baseline shall read
`.agents/skills/create-autosar-requirements/SKILL.md` and follow it. Keep new
requirements Draft and prepare them for review; authoring does not approve them.
The skill is plain Markdown and applies independently of model provider or tool.
If the tool does not discover skills automatically, open the file directly.
At the start of requirement creation, inform the user that the latest available
model suitable for critical source analysis with High reasoning effort is
recommended. This is a recommendation, not a prerequisite that blocks work.

## Teaching requirement discussions

When the user asks what a requirement means, why it exists, or how to implement it, read `.agents/skills/teach-requirement/SKILL.md` and follow it. Explain the approach and give code examples.

## Interactive requirement review

When starting or continuing an interactive requirement review, every LLM or coding
agent shall read `.agents/skills/review-requirements/SKILL.md` and follow it.
Review one requirement at a time; only explicit user agreement to its current
wording permits `Status: Agreed`. Questions and discussion are not agreement.
Changed wording requires renewed review and agreement.
At the start of review, recommend the latest available suitable model with High
reasoning effort, or the highest setting the provider supports, for critical
requirement and AUTOSAR source analysis. This recommendation does not block review.

## Development boundaries

- Keep requirements traceable to their IDs and planned verification.
- Preserve the distinction between AUTOSAR source behavior and AdaptivePi-specific decisions.
- For Releases, do not begin implementation until every requirement in that release has an agreed baseline (`Agreed`, or legacy `Approved`). Later `Implemented`/`Verified` states retain that acceptance only for unchanged agreed wording; open Draft/In Review revisions do not satisfy the gate.
- Follow the repository's existing formatting, CMake structure, tests, and pull-request workflow when making changes.
- When the user asks for an implementation, guide them to understand the requirement and implementation rather than changing repository code. (AI agents are not allowed to write production code directly; they are allowed to write and modify unit tests.)
- When writing or modifying unit tests, follow `.agents/skills/write-unit-tests/SKILL.md`.
