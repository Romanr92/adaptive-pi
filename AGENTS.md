# AdaptivePi agent instructions

AdaptivePi is an educational, clean-room Adaptive AUTOSAR-inspired project. It does not claim AUTOSAR conformance or use proprietary AUTOSAR implementation code.

## Source of truth

- Read `docs/requirements/README.md` and the relevant requirement file before discussing or implementing a requirement.
- Treat approved requirement text, its verification method, and its stated deviation as the implementation baseline.
- Consult `README.md` and `docs/` for information about the project contents, architecture, guides or requirements.
- Consult `docs/adr/`, relevant code, and tests for architectural and implementation context.
- Use conversation history when available, but do not let remembered decisions override the current repository. If they disagree, point out the difference.
- Do not add AUTOSAR features beyond the project's stated scope merely to match a specification.

## Teaching requirement discussions

When the user asks what a requirement means, why it exists, or how to implement it, read `.agents/skills/teach-requirement/SKILL.md` and follow it. Explain the approach before writing code unless the user explicitly asks for implementation.

## Development boundaries

- Keep requirements traceable to their IDs and planned verification.
- Preserve the distinction between AUTOSAR source behavior and AdaptivePi-specific decisions.
- For Release 3, do not begin implementation of a requirement unless that requirement status is `Approved`.
- Follow the repository's existing formatting, CMake structure, tests, and pull-request workflow when making changes.
- When the user asks for an implementation, guide them to understand the requirement and implementation rather than changing repository code. (AI Agents are not allowed to write code directly)