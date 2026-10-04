#!/usr/bin/env python3
"""Run registered ESBMC coverage experiments and render their bounded evidence (#62)."""

import argparse
from collections import defaultdict
from datetime import datetime, timezone
import hashlib
import html
import json
from pathlib import Path
import re
import shlex
import subprocess


def parse_coverage(raw, log):
    """Accept ESBMC 8.5 JSON, or its complete text summary without inventing locations."""
    if raw is not None:
        data = json.loads(raw)
        if not isinstance(data, dict) or data.get("coverage_type") != "branch":
            raise ValueError("Expected branch coverage")
        summary = data["summary"]
        if not isinstance(summary, dict):
            raise ValueError("Invalid summary")
        total, covered = summary["total"], summary["covered"]
        claims = data.get("claims", [])
        if not isinstance(claims, list):
            raise ValueError("Invalid claims")
        for claim in claims:
            if (not isinstance(claim, dict) or claim.get("status") not in ("covered", "uncovered")
                    or not isinstance(claim.get("file"), str)
                    or type(claim.get("line")) is not int or claim["line"] < 1
                    or not isinstance(claim.get("condition"), str)):
                raise ValueError("Invalid branch claim")
        if claims and (len(claims) != total
                       or sum(c["status"] == "covered" for c in claims) != covered):
            raise ValueError("Claims disagree with summary")
    else:
        match = re.search(r"Branches\s*:\s*(\d+)\s+Reached\s*:\s*(\d+)", log)
        if not match or "COVERAGE ANALYSIS COMPLETE" not in log:
            raise ValueError("No complete coverage summary")
        total, covered = map(int, match.groups())
        claims = []
    if type(total) is not int or type(covered) is not int or not 0 <= covered <= total:
        raise ValueError("Invalid coverage counts")
    return {"total": total, "covered": covered, "claims": claims}


def parse_properties(log):
    """Read ESBMC's located safety-property results, not coverage witnesses."""
    properties = []
    filename = None
    function = ""
    for line in log.splitlines():
        location = re.fullmatch(r"(.+), function (.+)", line)
        if location:
            filename, function = location.groups()
            continue
        result = re.fullmatch(r"\s+(PASSED|FAILED|UNKNOWN|UNREACHABLE)\s+\[([^]]+)\]\s+line\s+(\d+)\s+(.+)", line)
        if filename and result:
            status, identifier, number, description = result.groups()
            properties.append({"file": filename, "line": int(number), "function": function,
                               "condition": f"{identifier}: {description}", "kind": "property",
                               "status": {"PASSED": "covered", "FAILED": "uncovered"}.get(status, "unknown")})
    return properties


def execute(command, cwd, timeout):
    try:
        result = subprocess.run(command, cwd=cwd, capture_output=True, text=True,
                                errors="replace", timeout=timeout, check=False)
        return result.returncode, result.stdout + "\n" + result.stderr
    except subprocess.TimeoutExpired as error:
        def decode(value):
            return value.decode(errors="replace") if isinstance(value, bytes) else value or ""
        return "timeout", decode(error.stdout) + decode(error.stderr)
    except OSError as error:
        return "execution error", str(error)


def run_manifest(manifest_path, output):
    manifest = json.loads(manifest_path.read_text())
    root = Path(manifest["root"]).resolve()
    output.mkdir(parents=True, exist_ok=True)
    _, version = execute([manifest["executable"], "--version"], root, 15)
    runs = []
    for proof in manifest["proofs"]:
        command = proof["command"]
        name = proof["name"]
        # A private working directory prevents concurrent cov-report.json collisions.
        work = output / hashlib.sha256(name.encode()).hexdigest()[:16]
        work.mkdir(exist_ok=True)
        cov_file = work / "cov-report.json"
        cov_file.unlink(missing_ok=True)
        print(f"ESBMC report: {name}, exceptions {manifest['exceptions']}", flush=True)
        code, proof_log = execute(command, proof["directory"], proof["timeout"])
        verdict = ("passed" if code == 0 and re.search(
            r"^VERIFICATION SUCCESSFUL\s*$", proof_log, re.M) else
            "failed" if re.search(r"^VERIFICATION FAILED\s*$", proof_log, re.M) else "unknown")
        coverage_command = command + ["-I" + proof["directory"],
                                      "--branch-coverage-claims", "--cov-report-json"]
        cov_code, cov_log = execute(coverage_command, work, proof["timeout"])
        run = {"name": name, "exceptions": manifest["exceptions"], "version": version.strip(),
               "directory": str(work), "command": command, "coverage_command": coverage_command,
               "unwind": command[command.index("--unwind") + 1], "timeout": proof["timeout"],
               "proof_status": verdict, "proof_exit": code, "coverage_exit": cov_code,
               "proof_log": proof_log, "coverage_log": cov_log,
               "coverage": None, "error": ""}
        try:
            if cov_code != 0 or "COVERAGE ANALYSIS COMPLETE" not in cov_log:
                raise ValueError(f"Coverage incomplete (exit: {cov_code})")
            run["coverage"] = parse_coverage(
                cov_file.read_text() if cov_file.exists() else None, cov_log)
        except (ValueError, KeyError, TypeError) as error:
            run["error"] = str(error)
        (work / "proof.log").write_text(proof_log)
        (work / "coverage.log").write_text(cov_log)
        runs.append(run)
    bundle = {"root": str(root), "runs": runs}
    (output / "results.json").write_text(json.dumps(bundle, indent=2))
    return bundle


def escape(value):
    return html.escape(str(value), quote=True)


def ratio(covered, total):
    if not total:
        return '<span class="unknown">N/A (0 goals)</span>'
    percentage = covered * 100 / total
    level = "high" if percentage >= 90 else "medium" if percentage >= 75 else "low"
    return (f'<span class="{level}">{covered} / {total} ({percentage:.1f}%)</span>'
            f'<meter min="0" max="{total}" value="{covered}"></meter>')


def source_path(filename, directory, root):
    path = Path(filename)
    path = (path if path.is_absolute() else Path(directory) / path).resolve()
    try:
        relative = path.relative_to(root)
    except ValueError:
        return None
    parts = relative.parts
    if (len(parts) < 4 or parts[0] not in ("apps", "platform")
            or not {"src", "include"}.intersection(parts[2:-1])
            or {"proofs", "tests", "test"}.intersection(parts[2:-1])
            or relative.suffix not in (".h", ".hpp", ".hh", ".hxx", ".cpp", ".c", ".cc", ".cxx")
            or re.fullmatch(r"test_.*|.*_(?:test|tests|spec)", relative.stem)):
        return None
    return relative.as_posix()


def render(bundles, root, revision, errors=()):
    # Keep raw evidence intact; every displayed metric uses production goals only.
    runs = []
    excluded = 0
    for bundle in bundles:
        for original in bundle["runs"]:
            run = dict(original)
            coverage = run.get("coverage")
            if coverage is not None:
                claims = coverage.get("claims", [])
                if len(claims) != coverage["total"]:
                    run["coverage"] = None
                    run["error"] = "Production coverage unavailable: source locations are incomplete."
                else:
                    production = [c for c in claims if source_path(c["file"], run["directory"], root)]
                    excluded += len(claims) - len(production)
                    run["coverage"] = {"total": len(production),
                                       "covered": sum(c["status"] == "covered" for c in production),
                                       "claims": production}
            runs.append(run)
    mapped = defaultdict(lambda: defaultdict(list))
    for run in runs:
        for claim in (run.get("coverage") or {}).get("claims", []):
            filename = source_path(claim["file"], run["directory"], root)
            if filename:
                mapped[filename][claim["line"]].append((run, claim))
    for run in runs:
        # Only completed successful proof runs can contribute passed checks.
        for claim in parse_properties(run["proof_log"]):
            filename = source_path(claim["file"], run["directory"], root)
            if filename:
                if claim["status"] == "covered" and run["proof_status"] != "passed":
                    claim["status"] = "unknown"
                mapped[filename][claim["line"]].append((run, claim))
    # Include uninstrumented production files so missing harnesses are visible.
    for base in ("apps", "platform"):
        for path in (root / base).rglob("*"):
            filename = source_path(str(path), str(root), root)
            if path.is_file() and filename:
                mapped.setdefault(filename, {})
    total = sum(r["coverage"]["total"] for r in runs if r.get("coverage"))
    covered = sum(r["coverage"]["covered"] for r in runs if r.get("coverage"))
    unknown = sum(r.get("coverage") is None for r in runs)
    passed = sum(r["proof_status"] == "passed" for r in runs)
    properties = [c for lines in mapped.values() for entries in lines.values()
                  for _, c in entries if c.get("kind") == "property"]
    property_passed = sum(c["status"] == "covered" for c in properties)
    chunks = ['''<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESBMC Proof and Branch Coverage Report</title><style>
body{font:15px Arial,sans-serif;margin:20px;color:#17202a}h1{text-align:center}
a{color:#0645ad}table{border-collapse:collapse;width:100%;margin:16px 0}
th{background:#487fac;color:white;text-align:left}th,td{padding:5px 9px;border:1px solid #d7e1ed}
tbody tr:nth-child(even){background:#edf3fa}.high{background:#86df87}.medium{background:#ffff70}
.low{background:#ffacaa}.unknown{background:#e3e3e3}.high,.medium,.low,.unknown{padding:2px 5px}
meter{margin-left:12px;width:130px}summary{cursor:pointer}pre{white-space:pre-wrap;overflow-wrap:anywhere}
.source{font:13px monospace}.source td{padding:2px 6px}.source code{white-space:pre}
.source tr.high{background:#86df87}.source tr.medium{background:#ffff70}
.source tr.low{background:#ffacaa}
.source .number{width:45px;background:#eee}.source .goals{width:260px}
.no-proof{color:#725000;background:#fff1ca;padding:2px 5px;display:inline-block}
.scroll{overflow:auto}header,footer{border-block:2px solid #487fac;padding:12px 0}
input{padding:8px;width:min(600px,90%)}.notice{padding:10px;background:#fff1ca}
</style></head><body><h1>ESBMC Proof and Branch Coverage Report</h1><header>''',
              f'<p>Revision: <code>{escape(revision)}</code> · '
              f'Generated: {datetime.now(timezone.utc).isoformat(timespec="seconds")}</p>',
              f'<p>Safety proofs passed: {passed} / {len(runs)} · '
              f'Production safety checks passed: {ratio(property_passed, len(properties))} · '
              f'Branch goals reached: {ratio(covered, total)} · '
              f'Coverage runs unavailable: {unknown}</p>',
              '<p>Bounded reachability within registered harnesses and their assumptions. '
              'A reached branch is not a safety proof. Counts sum goals across harnesses and '
              'exception modes; they are not unique program branches. Only production sources '
              'are counted; proof harnesses, tests, and external libraries are excluded. Unreached means no witness '
              'within this model and bound, not necessarily unreachable in the program.</p>',
              '<p>Source details distinguish safety properties from branch reachability. '
              'Passed safety checks establish only the listed properties within the harness model and bounds, '
              'not correctness of every operation on a line. No branch goals does not mean no proof evidence. '
              'Instruction/line execution coverage is unavailable from this output. '
              'No proof evidence means no located safety check was recorded, not a failed proof. '
              'This label is not an executable-line classification; declarations may also lack checks. '
              'Branch not reached means no witness within the model and bound, not a runtime hit count. Thresholds: '
              '<span class="low">&lt;75%</span> <span class="medium">75–&lt;90%</span> '
              '<span class="high">≥90%</span>.</p></header>']
    if errors or unknown or not runs:
        chunks.append('<p class="notice">Incomplete report: missing or unsuccessful coverage runs '
                      'are excluded from percentages. ' + escape("; ".join(errors)) + '</p>')
    components = defaultdict(list)
    for filename, lines in mapped.items():
        component = "/".join(Path(filename).parts[:2])
        components[component].extend(c for entries in lines.values() for _, c in entries)
    chunks.append('<h2>Components</h2><table><thead><tr><th>Component</th>'
                  '<th>Branch goals reached</th><th>Safety checks passed</th></tr></thead><tbody>')
    for component, all_claims in sorted(components.items()):
        checks = [c for c in all_claims if c.get("kind") == "property"]
        claims = [c for c in all_claims if c.get("kind") != "property"]
        metric = (ratio(sum(c["status"] == "covered" for c in claims), len(claims))
                  if claims else '<span class="unknown">No branch goals</span>')
        chunks.append(f'<tr><td>{escape(component)}</td><td>{metric}</td><td>'
                      + ratio(sum(c['status'] == 'covered' for c in checks), len(checks)) + '</td></tr>')
    chunks.append('</tbody></table>')
    chunks.append('<h2>Proof runs</h2><table><thead><tr><th>Harness / configuration</th>'
                  '<th>Safety proof</th><th>Branch goals reached</th><th>Bounds / evidence</th>'
                  '</tr></thead><tbody>')
    for run in runs:
        coverage = run.get("coverage")
        status = run["proof_status"]
        level = {"passed": "high", "failed": "low"}.get(status, "unknown")
        bound_note = ('<br><span class="medium">Unwinding limit reported; inspect safety log</span>'
                      if status != "passed" and re.search(r"unwinding assertion", run["proof_log"], re.I)
                      else "")
        chunks.append(f'<tr><td>{escape(run["name"])}<br>Exceptions: {escape(run["exceptions"])}</td>'
                      f'<td><span class="{level}">{escape(status)}</span></td><td>'
                      + (ratio(coverage["covered"], coverage["total"]) if coverage else
                         '<span class="unknown">Unavailable</span>')
                      + f'</td><td>Unwind: {escape(run["unwind"])}; timeout: '
                      f'{escape(run["timeout"])}s per analysis{bound_note}<details><summary>Commands and logs</summary>'
                      f'<pre>{escape(run["version"])}\n{escape(run.get("error", ""))}\n'
                      f'Safety exit: {escape(run["proof_exit"])}\n'
                      f'{escape(shlex.join(run["command"]))}\n{escape(run["proof_log"])}\n'
                      f'Coverage exit: {escape(run["coverage_exit"])}\n'
                      f'{escape(shlex.join(run["coverage_command"]))}\n{escape(run["coverage_log"])}</pre>'
                      '</details></td></tr>')
    chunks.append('</tbody></table><h2>Source files</h2><label>Filter files '
                  '<input id="filter" type="search" placeholder="File or component name"></label>'
                  '<table id="files"><thead><tr><th>File / component</th><th>Branch goals reached</th>'
                  '<th>Safety checks passed</th><th>Lines with evidence</th></tr></thead><tbody>')
    sections = []
    for filename, lines in sorted(mapped.items()):
        anchor = "file-" + hashlib.sha256(filename.encode()).hexdigest()[:16]
        claims = [c for entries in lines.values() for _, c in entries if c.get("kind") != "property"]
        checks = [c for entries in lines.values() for _, c in entries if c.get("kind") == "property"]
        count = sum(c["status"] == "covered" for c in claims)
        metric = ratio(count, len(claims)) if claims else '<span class="unknown">No branch goals</span>'
        chunks.append(f'<tr><td><a href="#{anchor}">{escape(filename)}</a></td>'
                      f'<td>{metric}</td><td>'
                      + ratio(sum(c['status'] == 'covered' for c in checks), len(checks))
                      + f'</td><td>{len(lines)}</td></tr>')
        sections.append(f'<section id="{anchor}"><h3>{escape(filename)}</h3>'
                        '<a href="#files">Back to files</a><div class="scroll"><table class="source">'
                        '<thead><tr><th>Line</th><th>Checks / goals</th><th>Source</th></tr></thead><tbody>')
        try:
            source = (root / filename).read_text(errors="replace").splitlines()
        except OSError:
            source = []
            sections.append('<tr><td colspan="3">Source unavailable</td></tr>')
        for number, text in enumerate(source, 1):
            entries = lines.get(number, [])
            reached = sum(c["status"] == "covered" for _, c in entries)
            level = "" if not entries else "unknown" if any(c["status"] == "unknown" for _, c in entries) else "high" if reached == len(entries) else "low" if reached == 0 else "medium"
            checks = [c for _, c in entries if c.get("kind") == "property"]
            branches = [c for _, c in entries if c.get("kind") != "property"]
            details = ('<span class="no-proof">No proof evidence</span>'
                       if text.strip() and not checks else "")
            if entries:
                summaries = []
                if checks:
                    summaries.append(f"Safety checks passed: {sum(c['status'] == 'covered' for c in checks)} / {len(checks)}")
                if branches:
                    missing = sum(c["status"] != "covered" for c in branches)
                    summaries.append(f"Branches reached: {len(branches) - missing} / {len(branches)}; {missing} not reached")
                details += '<details><summary>' + '; '.join(summaries) + '</summary><ul>' 
                for run, claim in entries:
                    if claim.get("kind") == "property":
                        label = {"covered": "Safety check passed", "uncovered": "Safety check failed",
                                 "unknown": "Safety check inconclusive"}[claim["status"]]
                    else:
                        label = "Branch reached" if claim["status"] == "covered" else "Branch not reached within bound"
                    badge = 'high' if claim['status'] == 'covered' else 'low' if claim['status'] == 'uncovered' else 'unknown'
                    details += (f'<li><span class="{badge}">{label}</span>: {escape(claim["condition"])} '
                                f'({escape(run["name"])}, exceptions {escape(run["exceptions"])}, '
                                f'unwind {escape(run["unwind"])})</li>')
                details += '</ul></details>'
            sections.append(f'<tr id="{anchor}-L{number}" class="{level}">'
                            f'<td class="number"><a href="#{anchor}-L{number}">{number}</a></td>'
                            f'<td class="goals">{details}</td><td><code>{escape(text)}</code></td></tr>')
        sections.append('</tbody></table></div></section>')
    chunks.append('</tbody></table>')
    chunks.extend(sections)
    chunks.append(f'<footer>{excluded} non-production goals excluded from all coverage totals '
                  'and source views. Bounds are recorded per run; this report '
                  'does not infer which unreached goals would become reachable with a larger bound.</footer>'
                  '<script>document.getElementById("filter").addEventListener("input",function(){'
                  'for(const row of document.querySelectorAll("#files tbody tr"))'
                  'row.hidden=!row.cells[0].textContent.toLowerCase().includes(this.value.toLowerCase());'
                  '});</script></body></html>')
    return "\n".join(chunks)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument("--run", action="store_true", help="Configure and run local registered proofs")
    selection.add_argument("--manifest", type=Path, help="Run the CMake-generated JSON manifest")
    parser.add_argument("--exceptions", choices=("OFF", "ON"), default="OFF")
    parser.add_argument("--esbmc", type=Path, help="Use an existing ESBMC executable with --run")
    parser.add_argument("--input", type=Path, action="append", default=[], help="Merge a results.json bundle")
    parser.add_argument("--output", type=Path, help="Report directory (default: build/ESMBC-coverage)")
    parser.add_argument("--root", type=Path, default=Path.cwd())
    parser.add_argument("--revision", default="local working tree")
    parser.add_argument("--error", action="append", default=[])
    args = parser.parse_args()
    args.root = args.root.resolve()
    args.output = (args.output or args.root / "build/ESMBC-coverage").resolve()
    if not (args.run or args.manifest or args.input or args.error):
        parser.error("select --run, --manifest, --input, or --error")
    if args.run:
        configuration = args.output / "configuration"
        command = ["cmake", "-S", str(args.root), "-B", str(configuration), "-G", "Ninja",
                   "-DBUILD_TESTING=OFF", "-DADAPTIVE_PI_ENABLE_CLANG_TIDY=OFF",
                   "-DADAPTIVE_PI_ENABLE_ESBMC=ON", "-DADAPTIVE_PI_ESBMC_REQUIRE_PROOFS=OFF",
                   "-DADAPTIVE_PI_ESBMC_RUN_AT_CONFIGURE=OFF",
                   f"-DADAPTIVE_PI_ENABLE_EXCEPTIONS={args.exceptions}"]
        if args.esbmc:
            command.append(f"-DESBMC_EXECUTABLE={args.esbmc.resolve()}")
        subprocess.run(command, check=True)
        args.manifest = configuration / "esbmc/coverage.json"
    bundles = []
    if args.manifest:
        bundles.append(run_manifest(args.manifest, args.output / "evidence"))
    for path in args.input:
        bundles.append(json.loads(path.read_text()))
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "index.html").write_text(render(
        bundles, args.root.resolve(), args.revision, args.error))
    print(f"Report: {args.output / 'index.html'}")


if __name__ == "__main__":
    main()
