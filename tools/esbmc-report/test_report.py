"""Regression tests for issue #62's parser, evidence handling, and HTML output."""

import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import esbmc_cov_to_html as report


class Issue62ReportTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / "platform/demo/src/branch.cpp"
        self.source.parent.mkdir(parents=True)
        self.source.write_text('int main() {\n  if (x < 0) return 1; // <script>alert(1)</script>\n}\n')
        self.data = {
            "coverage_type": "branch", "summary": {"total": 2, "covered": 1},
            "claims": [
                {"file": str(self.source), "line": 2, "column": 3, "function": "main",
                 "condition": "x < 0", "status": "covered"},
                {"file": str(self.source), "line": 2, "column": 3, "function": "main",
                 "condition": "!(x < 0)", "status": "uncovered"},
            ],
        }
        self.run = {
            "name": "demo", "exceptions": "OFF", "version": "ESBMC 8.5",
            "directory": str(self.root), "unwind": "4", "timeout": 1,
            "command": ["esbmc", str(self.source), "--unwind", "4"],
            "coverage_command": ["esbmc", "--branch-coverage-claims"],
            "proof_status": "passed", "proof_exit": 0, "coverage_exit": 0,
            "proof_log": "VERIFICATION SUCCESSFUL", "coverage_log": "COVERAGE ANALYSIS COMPLETE",
            "coverage": report.parse_coverage(json.dumps(self.data), ""), "error": "",
        }

    def test_issue62_json_and_text_counts(self):
        """Arrange equivalent outputs; parse each; expect equal totals without invented text locations."""
        for raw, log in ((json.dumps(self.data), ""),
                         (None, "Branches : 2\nReached : 1\nCOVERAGE ANALYSIS COMPLETE")):
            with self.subTest(raw=raw):
                parsed = report.parse_coverage(raw, log)
                self.assertEqual((parsed["total"], parsed["covered"]), (2, 1))
                if raw is None:
                    self.assertEqual(parsed["claims"], [])

    def test_issue62_rejects_invalid_or_incomplete_evidence(self):
        """Arrange malformed evidence; parse it; expect rejection instead of a green result."""
        invalid = ["{", "null", "[]", json.dumps({**self.data, "summary": None}),
                   json.dumps({**self.data, "claims": [None]}), json.dumps({**self.data, "coverage_type": "condition"}),
                   json.dumps({**self.data, "summary": {"total": 2, "covered": 3}}),
                   json.dumps({**self.data, "summary": {"total": 2, "covered": 0}}),
                   json.dumps({**self.data, "summary": {"total": True, "covered": 1}})]
        for raw in invalid:
            with self.subTest(raw=raw), self.assertRaises((ValueError, KeyError)):
                report.parse_coverage(raw, "")
        with self.assertRaises(ValueError):
            report.parse_coverage(None, "Branches : 2\nReached : 1\n")

    def test_issue62_source_view_escapes_and_expands_branch_details(self):
        """Arrange partial reachability and HTML in source; render; expect safe source and branch details."""
        output = report.render([{"runs": [self.run]}], self.root, "revision<1>")
        for expected in ('class="medium"', "Branch not reached within bound", "unwind 4",
                         "&lt;script&gt;alert(1)&lt;/script&gt;", "revision&lt;1&gt;",
                         "platform/demo", "Back to files"):
            self.assertIn(expected, output)
        self.assertNotIn("<script>alert(1)</script>", output)

    def test_issue62_merges_runs_without_merging_proof_verdicts(self):
        """Arrange two modes with different verdicts; render together; expect both results and summed goals."""
        second = {**self.run, "exceptions": "ON", "proof_status": "failed", "proof_exit": 1}
        output = report.render([{"runs": [self.run]}, {"runs": [second]}], self.root, "test")
        self.assertIn("Safety proofs passed: 1 / 2", output)
        self.assertIn("2 / 4 (50.0%)", output)
        self.assertIn('>failed</span>', output)
        self.assertIn("Exceptions: ON", output)

    def test_issue62_missing_and_zero_goals_are_not_full_coverage(self):
        """Arrange missing or zero-goal runs; render; expect unavailable or N/A instead of 100%."""
        for coverage in (None, {"total": 0, "covered": 0, "claims": []}):
            with self.subTest(coverage=coverage):
                run = {**self.run, "coverage": coverage}
                output = report.render([{"runs": [run]}], self.root, "test")
                self.assertIn("N/A (0 goals)", output)
                self.assertNotIn("100.0%", output)
                if coverage is None:
                    self.assertIn("Incomplete report", output)

    def test_issue62_omits_external_and_symlink_sources(self):
        """Arrange a source symlink escaping the root; map it; expect no external source disclosure."""
        link = self.source.parent / "external.cpp"
        link.symlink_to("/etc/passwd")
        for path in ("/etc/passwd", str(link), "../outside.cpp"):
            with self.subTest(path=path):
                self.assertIsNone(report.source_path(path, str(self.root), self.root))

    def test_issue62_uninstrumented_sources_are_visible(self):
        """Arrange a production file with no goals; render; expect a neutral source entry."""
        output = report.render([], self.root, "test", ["tool unavailable"])
        self.assertIn("platform/demo/src/branch.cpp", output)
        self.assertIn("No branch goals", output)
        self.assertIn("tool unavailable", output)

    def test_issue62_only_production_goals_contribute_to_coverage(self):
        """Arrange mixed production, proof, test and library goals; render; expect production-only totals."""
        excluded = ["platform/demo/proofs/proof.cpp", "platform/demo/tests/test.cpp",
                    "platform/demo/src/test_helper.cpp", "platform/demo/src/proofs/helper.h",
                    "apps/demo/proofs/include/helper.h", "vendor/library/src/code.cpp"]
        claims = list(self.run["coverage"]["claims"])
        for filename in excluded:
            path = self.root / filename
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("// excluded source")
            claims.append({**claims[0], "file": str(path)})
        run = {**self.run, "coverage": {"total": len(claims), "covered": 7, "claims": claims}}
        output = report.render([{"runs": [run]}], self.root, "test")
        self.assertIn("Branch goals reached: <span class=\"low\">1 / 2 (50.0%)", output)
        self.assertIn("6 non-production goals excluded", output)
        self.assertIn("Safety proofs passed: 1 / 1", output)
        for filename in excluded:
            self.assertNotIn(filename, output)
        self.assertEqual(run["coverage"]["total"], 8)

    def test_issue62_unlocated_totals_cannot_be_production_coverage(self):
        """Arrange text-only totals; render; expect unavailable production coverage, not unfiltered counts."""
        run = {**self.run, "coverage": {"total": 10, "covered": 10, "claims": []}}
        output = report.render([{"runs": [run]}], self.root, "test")
        self.assertIn("Coverage runs unavailable: 1", output)
        self.assertIn("source locations are incomplete", output)
        self.assertNotIn("100.0%", output)

    def test_issue62_production_properties_are_shown_without_branch_goals(self):
        """Arrange located proof results and no branches; render; expect production safety evidence."""
        log = (f"{self.source}, function main\n"
               "  PASSED [main.bounds.1] line 2 array bounds check\n"
               f"{self.root}/platform/demo/proofs/proof.cpp, function main\n"
               "  PASSED [main.assertion.1] line 4 assertion contract\n")
        run = {**self.run, "proof_log": log, "coverage": {"total": 0, "covered": 0, "claims": []}}
        output = report.render([{"runs": [run]}], self.root, "test")
        self.assertIn("Production safety checks passed: <span class=\"high\">1 / 1", output)
        self.assertIn("Safety check passed</span>: main.bounds.1: array bounds check", output)
        self.assertIn("No branch goals", output)
        self.assertNotIn("Safety check passed</span>: main.assertion.1", output)
        run["proof_status"] = "unknown"
        output = report.render([{"runs": [run]}], self.root, "test")
        self.assertIn("Safety check inconclusive", output)
        self.assertNotIn("Safety check passed</span>: main.bounds.1", output)

    def test_issue62_missing_proof_and_unreached_branch_are_explicit(self):
        """Arrange branch evidence without safety checks; render; expect distinct missing-proof and branch labels."""
        output = report.render([{"runs": [self.run]}], self.root, "test")
        self.assertIn("No proof evidence</span>", output)
        self.assertIn("Branches reached: 1 / 2; 1 not reached", output)
        self.assertIn('class="low">Branch not reached within bound</span>', output)
        self.assertNotIn("Safety checks passed: 1 / 2", output)

    def test_issue62_timeout_retains_partial_log(self):
        """Arrange a timed-out subprocess; execute; expect unknown status and retained diagnostics."""
        error = subprocess.TimeoutExpired(["esbmc"], 1, output=b"partial", stderr=b"diagnostic")
        with patch.object(report.subprocess, "run", side_effect=error):
            status, log = report.execute(["esbmc"], self.root, 1)
        self.assertEqual(status, "timeout")
        self.assertEqual(log, "partialdiagnostic")

    def test_issue62_unwinding_failure_is_visible(self):
        """Arrange a bound failure; render; expect an explicit bound warning beside the result."""
        run = {**self.run, "proof_status": "failed", "proof_log": "unwinding assertion loop 0"}
        output = report.render([{"runs": [run]}], self.root, "test")
        self.assertIn("Unwinding limit reported", output)

    def test_issue62_coverage_success_does_not_hide_safety_failure(self):
        """Arrange complete coverage and a failed safety proof; run; expect independent verdicts."""
        manifest = self.root / "manifest.json"
        manifest.write_text(json.dumps({"root": str(self.root), "exceptions": "OFF",
            "executable": "esbmc", "proofs": [{"name": "demo", "command": self.run["command"],
                "directory": str(self.root), "timeout": 1}]}))

        def execute(command, directory, timeout):
            if "--version" in command:
                return 0, "8.5"
            if "--cov-report-json" in command:
                (Path(directory) / "cov-report.json").write_text(json.dumps(self.data))
                return 0, "COVERAGE ANALYSIS COMPLETE"
            return 1, "VERIFICATION FAILED"

        with patch.object(report, "execute", side_effect=execute):
            bundle = report.run_manifest(manifest, self.root / "evidence")
        self.assertEqual(bundle["runs"][0]["proof_status"], "failed")
        self.assertEqual(bundle["runs"][0]["coverage"]["covered"], 1)

    def test_issue62_runner_discards_stale_coverage_and_continues(self):
        """Arrange stale JSON and a timeout; run two proofs; expect no reused evidence and both records."""
        manifest = self.root / "manifest.json"
        manifest.write_text(json.dumps({"root": str(self.root), "exceptions": "OFF",
            "executable": "esbmc", "proofs": [
                {"name": name, "command": self.run["command"], "directory": str(self.root), "timeout": 1}
                for name in ("one", "two")]}))
        output = self.root / "evidence"
        stale = output / report.hashlib.sha256(b"one").hexdigest()[:16] / "cov-report.json"
        stale.parent.mkdir(parents=True)
        stale.write_text(json.dumps(self.data))
        results = [(0, "8.5"), (0, "VERIFICATION SUCCESSFUL"), ("timeout", "partial"),
                   (1, "VERIFICATION FAILED"), (1, "parse error")]
        with patch.object(report, "execute", side_effect=results):
            bundle = report.run_manifest(manifest, output)
        self.assertFalse(stale.exists())
        self.assertEqual(len(bundle["runs"]), 2)
        self.assertTrue(all(r["coverage"] is None for r in bundle["runs"]))
        self.assertEqual(bundle["runs"][1]["proof_status"], "failed")
        self.assertTrue((output / "results.json").is_file())


if __name__ == "__main__":
    unittest.main()
