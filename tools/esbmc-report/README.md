# ESBMC HTML reports (issue #62)

This standard-library Python tool runs the registered ESBMC safety proofs and a
separate branch-reachability analysis, then builds a self-contained HTML report.
It provides component and file summaries, searchable file navigation, numbered
source views, expandable branch details, bounds, proof verdicts, and full logs.
No Python packages or web server are required.

## Create a report locally

From the repository root:

```bash
python3 tools/esbmc-report/esbmc_cov_to_html.py --run
```

Open **`build/ESMBC-coverage/index.html`** in your browser. The folder spelling is
intentional. Configuration and raw evidence also stay under `build/ESMBC-coverage/`;
generated reports are not committed.

Prerequisites: Python 3.10+, CMake 3.25+, Ninja, a host C++ compiler, and the
[ESBMC install prerequisites](../../docs/guides/esbmc.md#install-and-run-esbmc).
CMake reuses ESBMC from PATH or installs the repository-pinned version if absent.
To use an existing installation explicitly:

```bash
python3 tools/esbmc-report/esbmc_cov_to_html.py --run \
  --esbmc build/debug-esbmc-proofs/tools/esbmc/bin/esbmc
```

The default local configuration disables exceptions. CI runs both supported
exception modes. `--exceptions ON` explicitly selects an exception-enabled run;
use a separate `--output` directory when retaining more than one configuration.
`--root /path/to/adaptive-pi` supports invocation from another directory.

An existing ESBMC-enabled CMake configuration also exposes:

```bash
cmake --build build/debug-esbmc-proofs --target esbmc-coverage-report
```

Reconfigure that build once if the target is not yet present. The target writes
to `build/ESMBC-coverage/` by default. Override with the CMake cache setting
`ADAPTIVE_PI_ESBMC_REPORT_DIR`. The ordinary strict proof preset still runs and
requires successful proofs during configuration. For an informational report
even when proofs fail, use the Python `--run` command above; it disables
configure-time proof execution in its own configuration and records each result.

## Data and interpretation

- Each harness runs twice: its ordinary safety command, then that command plus
  `--branch-coverage-claims --cov-report-json`. Each analysis gets the registered
  timeout and unwind depth. Coverage does not reuse successful-proof caches.
- ESBMC 8.5 JSON supplies branch goals, conditions, source locations, and covered
  or uncovered status. Complete text summaries are a fallback when JSON is
  absent; text-only totals without source locations are unavailable for
  production coverage because their scope cannot be filtered. Malformed JSON,
  timeouts, execution errors, or missing completion markers produce unavailable
  coverage, not zero or 100%.
- `--show-cov-stats` from the issue proposal is not exposed by the installed
  ESBMC 8.5 command help. This tool uses the observed `cov-report.json` schema
  and `Branches` / `Reached` text summary instead.
- Coverage percentages describe **reached instrumented branch goals**, not all
  reachable program branches, tested lines, or proven-safe source lines. Goals
  are counted only in production `src/` and `include/` files under `apps/` and
  `platform/`. Proof harnesses, tests, and external libraries are excluded from
  every displayed coverage percentage and source view. Raw evidence is retained
  unchanged, and harness verification results remain in the proof summary.
- Source lines with all goals reached are green, mixed goals amber, and no goals
  reached red. Lines without branch evidence are neutral. Production files with
  no instrumented goals are listed explicitly. Function, instruction, and full
  line-execution coverage are not available from this output.
- An unreached goal may be excluded by assumptions, be unreachable, or require
  a larger bound. The report displays the actual unwind depth and logs; it
  cannot attribute each unreached goal to bound exhaustion. A safety-proof
  unwinding failure is identified separately when reported in its log.
- Totals sum goal instances across harnesses and configurations. They are not a
  deduplicated measure of unique program branches. External library goals are excluded
  from the overall summary and source views.
- A coverage witness is separate from a successful safety proof. Both results
  and their commands appear in the report. A failed proof does not suppress
  other harnesses or stop report creation.

Each run has a private evidence directory, avoiding `cov-report.json` collisions.
`evidence/results.json` contains normalized results and logs. To merge saved
runs from the **same source revision** without rerunning ESBMC:

```bash
python3 tools/esbmc-report/esbmc_cov_to_html.py \
  --input build/report-OFF/evidence/results.json \
  --input build/report-ON/evidence/results.json \
  --revision "$(git rev-parse HEAD)"
```

Use a checkout of that revision for accurate source views. HTML embeds all
styles, scripts, source, and logs and can be copied as a single file.

## Publication

The existing **Coverage Pages** workflow runs when a pull request is merged into
`main`. It generates both exception configurations, combines their evidence, and
publishes `/esbmc/` alongside `/coverage/` in the same Pages deployment. The
homepage links to both reports. The workflow also retains the ESBMC HTML, raw
JSON, logs, and command manifests as a downloadable artifact for 14 days.

This reporting addition is informational and adds no required PR check. Proof
failures and unavailable coverage appear in the report; installation or report
step failures produce an unavailable-report page so unit-test coverage can still
be deployed. Existing Host CI proof enforcement is unchanged.

## Verify the tool

```bash
python3 -m unittest discover -s tools/esbmc-report -p 'test_*.py' -v
```

Tests cover output parsing, malformed and incomplete data, aggregate counts,
HTML escaping, source path containment, missing instrumentation, timeouts, and
stale output rejection. For an end-to-end check, run the local command above
and inspect its generated report.

## Production safety properties versus branch coverage

The report also parses located safety-property results from the ordinary ESBMC
proof log. Component and file summaries display **Safety checks passed** separately
from **Branch goals reached**. Expand a source line to see the exact property,
its verdict, harness, exception mode, and unwind bound. Only production-source
locations contribute; harness assertions remain in the raw proof evidence.

A file can have no branch goals and still contain successfully checked safety
properties. “No branch goals” does not mean nothing was verified. Conversely,
a passed check establishes only that particular property in the harness model;
it is not a percentage of all code proven correct. Files without either kind of
located evidence remain neutral. Passed properties from an incomplete or failed
proof run are conservatively displayed as inconclusive.

Source rows without a located safety check show **No proof evidence** (including
rows with only branch-reachability evidence). This is not a claim that the line
failed verification or is executable. Blank lines remain unlabelled. Branch
summaries show reached and not-reached goal counts; expand them for conditions,
red **Branch not reached within bound** labels, and harness/bound context.
Branches absent from ESBMC instrumentation cannot be classified as not taken.
