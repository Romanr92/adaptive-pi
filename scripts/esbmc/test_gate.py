#!/usr/bin/env python3
"""Exercise issue #42's gate with real proofs and isolated failure fixtures.

No network or production-source edits: each test owns a temporary CMake project.
The real solver checks passing/failing assertions; test doubles exercise process
errors and timeouts. Run with --esbmc /absolute/path/to/the/pinned/esbmc.
"""

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

REPOSITORY = Path(__file__).resolve().parents[2]
PASSING_PROOF = """#include <cassert>
extern unsigned int nondet_uint();
int main() { const auto value = nondet_uint(); assert(value / 2 <= value); }
"""


class GateIntegrationTests(unittest.TestCase):
    esbmc: Path

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="adaptive-pi-esbmc-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / "source with spaces"
        self.build = self.root / "build with spaces"
        self.source.mkdir()
        module = (REPOSITORY / "cmake/ESBMC.cmake").as_posix()
        (self.source / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.25)\n"
            "project(GateIntegration LANGUAGES NONE)\n"
            f'include("{module}")\n'
            "adaptive_pi_run_esbmc_proofs()\n"
        )

    def proof(self, root, name, content=PASSING_PROOF, options="", timeout=30):
        directory = self.source / root / "fixture/proofs"
        directory.mkdir(parents=True, exist_ok=True)
        (directory / "CMakeLists.txt").write_text(
            f"add_esbmc_proof({name} SOURCES proof.cpp UNWIND 4 TIMEOUT {timeout} {options})\n"
        )
        source = directory / "proof.cpp"
        source.write_text(content)
        return source

    def invoke(self, *command, success=True, env=None):
        result = subprocess.run(
            command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            timeout=90, env=env,
        )
        self.assertEqual(result.returncode == 0, success, result.stdout)
        return result.stdout

    def configure(self, *options, success=True, executable=None):
        return self.invoke(
            "cmake", "-S", str(self.source), "-B", str(self.build), "-G", "Ninja",
            f"-DESBMC_EXECUTABLE={executable or self.esbmc}",
            "-DADAPTIVE_PI_ENABLE_ESBMC=ON",
            "-DADAPTIVE_PI_ESBMC_REQUIRE_PROOFS=ON", *options, success=success,
        )

    def log(self, name):
        return (self.build / "esbmc/logs" / f"{name}.log").read_text()

    def checker(self, body, solvers="z3"):
        checker = self.root / "checker"
        checker.write_text(
            f"#!{sys.executable}\nimport sys, time\n"
            "if '--version' in sys.argv:\n print('test checker'); sys.exit(0)\n"
            f"if '--list-solvers' in sys.argv:\n print('Available solvers: {solvers}'); sys.exit(0)\n"
            + body + "\n"
        )
        checker.chmod(0o755)
        return checker

    def test_both_roots_pass_with_z3_and_build_target(self):
        for root, name in (("platform", "core-proof"), ("apps", "app-proof")):
            self.proof(root, name)
        self.configure()
        self.invoke("cmake", "--build", str(self.build), "--target", "run-esbmc")
        for name in ("core-proof", "app-proof"):
            with self.subTest(proof=name):
                self.assertIn("Solving with solver Z3", self.log(name))
                self.assertIn("VERIFICATION SUCCESSFUL", self.log(name))
        summary = (self.build / "esbmc/logs/summary.md").read_text()
        self.assertIn("| core-proof | PASSED |", summary)
        self.assertIn("| app-proof | PASSED |", summary)
        self.assertIn("'--z3'", summary)

    def test_report_target_exports_commands_and_runs_from_external_project(self):
        """Arrange an external project with spaces; build each requested mode; verify manifest and evidence."""
        component = self.source / "platform/fixture"
        (component / "src").mkdir(parents=True)
        (component / "include").mkdir()
        (component / "src/branch.cpp").write_text(
            '#include "branch.hpp"\nint classify(int value) { if (value > 0) return 1; return 0; }\n')
        (component / "include/branch.hpp").write_text('int classify(int value);\n')
        self.proof("platform", "report-proof",
                   '#include "branch.hpp"\n#include <cassert>\nextern int nondet_int();\n'
                   'int main() { int x = nondet_int(); assert(classify(x) == (x > 0)); }\n')
        (component / "proofs/CMakeLists.txt").write_text(
            'add_esbmc_proof(report-proof SOURCES proof.cpp ../src/branch.cpp '
            'INCLUDE_DIRECTORIES ../include DEFINITIONS [=[REPORT_LABEL="with spaces"]=] '
            'UNWIND 4 TIMEOUT 30)\n')
        for mode in self.report_exception_modes:
            with self.subTest(exceptions=mode):
                self.build = self.root / f"build with spaces {mode}"
                output = self.root / f"report with spaces {mode}"
                self.configure("-DADAPTIVE_PI_ESBMC_REQUIRE_PROOFS=OFF",
                               "-DADAPTIVE_PI_ESBMC_RUN_AT_CONFIGURE=OFF",
                               f"-DADAPTIVE_PI_ENABLE_EXCEPTIONS={mode}",
                               f"-DADAPTIVE_PI_ESBMC_REPORT_DIR={output}")
                manifest = json.loads((self.build / "esbmc/coverage.json").read_text())
                command = manifest["proofs"][0]["command"]
                self.assertEqual(manifest["root"], str(self.source))
                self.assertEqual(manifest["exceptions"], mode)
                self.assertEqual(command[0], str(self.esbmc))
                self.assertIn(str(component / "src/branch.cpp"), command)
                self.assertIn('-DREPORT_LABEL="with spaces"', command)
                self.assertIn('-fexceptions' if mode == 'ON' else '-fno-exceptions', command)
                self.assertIn(f'-DADAPTIVE_PI_EXCEPTIONS_ENABLED={int(mode == "ON")}', command)
                self.invoke("cmake", "--build", str(self.build), "--target", "esbmc-coverage-report")
                bundle = json.loads((output / "evidence/results.json").read_text())
                self.assertEqual(len(bundle["runs"]), 1)
                run = bundle["runs"][0]
                self.assertEqual(run["command"], command)
                self.assertEqual(run["exceptions"], mode)
                self.assertEqual(run["proof_status"], "passed")
                self.assertEqual(run["coverage_exit"], 0)
                self.assertTrue(run["coverage"]["claims"])
                self.assertTrue(any(c["file"] == str(component / "src/branch.cpp")
                                    for c in run["coverage"]["claims"]))
                self.assertTrue((Path(run["directory"]) / "cov-report.json").is_file())
                html = (output / "index.html").read_text()
                self.assertIn("platform/fixture/src/branch.cpp", html)
                self.assertIn("Safety proofs passed: 1 / 1", html)
                self.assertIn(f"Exceptions: {mode}", html)

    def test_deliberately_failing_proof_fails_configure_and_runs_remaining_proofs(self):
        self.proof("apps", "failing", "#include <cassert>\nint main() { assert(false); }\n")
        self.proof("platform", "passing")
        output = self.configure(success=False)
        self.assertIn("ESBMC FAILED: failing", output)
        self.assertIn("ESBMC PASSED: passing", output)
        self.assertIn("VERIFICATION FAILED", self.log("failing"))
        summary = (self.build / "esbmc/logs/summary.md").read_text()
        self.assertIn("| failing | FAILED |", summary)
        self.assertIn("proof.cpp", summary)

    def test_harness_edit_fails_build_without_reconfiguration(self):
        source = self.proof("apps", "edited")
        self.configure()
        source.write_text("#include <cassert>\nint main() { assert(false); }\n")
        self.invoke("cmake", "--build", str(self.build), "--target", "run-esbmc", success=False)
        self.assertIn("VERIFICATION FAILED", self.log("edited"))

    def test_timeout_fails(self):
        self.proof("platform", "timeout", timeout=1)
        self.configure(executable=self.checker("time.sleep(5)"), success=False)
        self.assertIn("timeout", self.log("timeout").lower())

    def test_zero_exit_without_success_and_nonzero_with_success_both_fail(self):
        self.proof("platform", "result")
        for body in ("sys.exit(0)", "print('VERIFICATION SUCCESSFUL'); sys.exit(1)"):
            with self.subTest(body=body):
                self.configure(executable=self.checker(body), success=False)

    def test_missing_executable_after_configuration_fails_build(self):
        self.proof("apps", "missing")
        checker = self.checker("print('VERIFICATION SUCCESSFUL')")
        self.configure(executable=checker)
        checker.unlink()
        self.invoke("cmake", "--build", str(self.build), "--target", "run-esbmc", success=False)
        self.assertIn("Result:", self.log("missing"))

    def test_missing_source_and_empty_suite_fail(self):
        self.assertIn("No ESBMC proofs registered", self.configure(success=False))
        self.proof("apps", "missing-source").unlink()
        self.assertIn("missing source", self.configure(success=False))

    def test_disabled_strict_gate_fails(self):
        output = self.configure("-DADAPTIVE_PI_ENABLE_ESBMC=OFF", success=False)
        self.assertIn("requires ADAPTIVE_PI_ENABLE_ESBMC=ON", output)

    def test_solver_without_z3_and_conflicting_option_fail(self):
        self.proof("platform", "solver", options="OPTIONS --bitwuzla")
        self.assertIn("unsupported OPTIONS argument", self.configure(success=False))
        checker = self.checker("sys.exit(0)", solvers="bitwuzla")
        self.assertIn("must provide the Z3 solver", self.configure(executable=checker, success=False))

    def test_unsafe_and_runner_owned_options_are_rejected_before_verification(self):
        checker = self.checker("raise RuntimeError('checker must not run')")
        for option in (
            "--no-pointer-check", "--no-unwinding-assertions", "--no-assertions",
            "--no-bounds-check", "--no-div-by-zero-check", "--unwind 1",
            "--unwind=1", "--z3", "--std c++11", "-fexceptions",
            "-fno-exceptions", "-DNDEBUG", "--function other_entry",
            "--overflow-check --no-assertions", "--unknown-future-option",
        ):
            with self.subTest(option=option):
                self.proof("apps", "unsafe", options=f"OPTIONS {option}")
                output = self.configure(executable=checker, success=False)
                self.assertIn("unsupported OPTIONS argument", output)
                self.assertNotIn("checker must not run", output)
                self.assertFalse((self.build / "esbmc/logs/unsafe.log").exists())

    def test_reserved_and_malformed_definitions_are_rejected(self):
        for definition in (
            "NDEBUG", "NDEBUG=0", "NDEBUG=1", "assert=ignored",
            "ADAPTIVE_PI_EXCEPTIONS_ENABLED=1", "ADAPTIVE_PI_EXCEPTIONS_ENABLED=0",
            "__EXCEPTIONS=1", "__cplusplus=201703L", "-DNDEBUG", "assert(x)=0",
        ):
            with self.subTest(definition=definition):
                self.proof("apps", "unsafe", options=f'DEFINITIONS "{definition}"')
                output = self.configure(success=False)
                self.assertTrue("reserved definition" in output or "DEFINITIONS requires" in output, output)
                self.assertFalse((self.build / "esbmc/logs/unsafe.log").exists())

    def test_additional_checks_and_custom_definition_verify_with_real_solver(self):
        self.proof(
            "apps", "safe-options",
            "#include <cassert>\nint main() { assert(PROOF_LIMIT == 4); }\n",
            options="DEFINITIONS PROOF_LIMIT=4 OPTIONS --overflow-check --unsigned-overflow-check",
        )
        self.configure()
        self.assertIn("Solving with solver Z3", self.log("safe-options"))

    def test_cache_reuses_unchanged_proof_but_runs_changed_harness(self):
        app = self.proof("apps", "app")
        self.proof("platform", "core")
        cache = f"-DADAPTIVE_PI_ESBMC_CACHE_DIR={self.root / 'cache'}"
        self.configure(cache)
        output = self.configure(cache)
        self.assertIn("ESBMC REUSED: app", output)
        self.assertIn("ESBMC REUSED: core", output)
        app.write_text(PASSING_PROOF + "// changed harness\n")
        output = self.configure(cache)
        self.assertIn("ESBMC PASSED: app", output)
        self.assertIn("ESBMC REUSED: core", output)

    def test_cache_invalidates_added_changed_and_deleted_implementation_inputs(self):
        self.proof("platform", "core")
        cache = f"-DADAPTIVE_PI_ESBMC_CACHE_DIR={self.root / 'cache'}"
        self.configure(cache)
        header = self.source / "platform/fixture/include/state.h"
        header.parent.mkdir()
        for contents in ("// initial dependency\n", "// changed dependency\n"):
            with self.subTest(contents=contents):
                header.write_text(contents)
                self.assertIn("ESBMC PASSED: core", self.configure(cache))
        # Remove any previous evidence of the original empty tree, then delete.
        for entry in (self.root / 'cache').glob('*.log'):
            entry.unlink()
        self.configure(cache)
        header.unlink()
        self.assertIn("ESBMC PASSED: core", self.configure(cache))

    def test_external_dependency_and_bound_invalidate_cache(self):
        dependency = self.source / "contract.txt"
        dependency.write_text("first contract")
        source = self.proof("apps", "app", options=f'DEPENDS "{dependency}"')
        cache = f"-DADAPTIVE_PI_ESBMC_CACHE_DIR={self.root / 'cache'}"
        self.configure(cache)
        dependency.write_text("second contract")
        self.assertIn("ESBMC PASSED: app", self.configure(cache))
        manifest = source.parent / "CMakeLists.txt"
        manifest.write_text(manifest.read_text().replace("UNWIND 4", "UNWIND 5"))
        self.assertIn("ESBMC PASSED: app", self.configure(cache))
        self.assertIn("ESBMC PASSED: app", self.configure("-DADAPTIVE_PI_ESBMC_CACHE_DIR="))

    def test_failures_are_never_cached(self):
        self.proof("apps", "bad", "#include <cassert>\nint main() { assert(false); }\n")
        cache = f"-DADAPTIVE_PI_ESBMC_CACHE_DIR={self.root / 'cache'}"
        for attempt in range(2):
            with self.subTest(attempt=attempt):
                output = self.configure(cache, success=False)
                self.assertNotIn("ESBMC REUSED", output)
                self.assertEqual(list((self.root / 'cache').glob('*.log')), [])

    def test_false_constant_names_run_and_cannot_hide_failures(self):
        for name in ("OFF", "NO", "0", "FALSE", "IGNORE", "NOTFOUND", "proof-NOTFOUND", "off"):
            with self.subTest(name=name):
                self.proof("apps", name)
                self.assertIn(f"ESBMC PASSED: {name}", self.configure())
                self.proof("apps", name, "#include <cassert>\nint main() { assert(false); }\n")
                output = self.configure(success=False)
                self.assertIn("proofs failed or were inconclusive", output)
                self.assertIn(f"ESBMC FAILED: {name}", output)

    def test_false_constant_failure_with_another_passing_proof_fails(self):
        self.proof("apps", "OFF", "#include <cassert>\nint main() { assert(false); }\n")
        self.proof("platform", "passing")
        self.assertIn("proofs failed or were inconclusive", self.configure(success=False))

    def test_false_constant_unknown_argument_is_rejected(self):
        self.proof("apps", "invalid", options="OFF")
        self.assertIn("check argument names", self.configure(success=False))

    def test_local_helper_header_change_invalidates_cache_at_build_time(self):
        source = self.proof("apps", "helper", '#include "helper.hpp"\nint main() { check(); }\n')
        header = source.parent / "helper.hpp"
        header.write_text("#include <cassert>\ninline void check() { assert(true); }\n")
        self.configure(f"-DADAPTIVE_PI_ESBMC_CACHE_DIR={self.root / 'cache'}")
        header.write_text("#include <cassert>\ninline void check() { assert(false); }\n")
        output = self.invoke("cmake", "--build", str(self.build), "--target", "run-esbmc", success=False)
        self.assertIn("ESBMC FAILED: helper", output)
        self.assertNotIn("ESBMC REUSED", output)

    def test_corrupted_cache_result_is_not_reused(self):
        self.proof("apps", "cached")
        cache = self.root / "cache"
        option = f"-DADAPTIVE_PI_ESBMC_CACHE_DIR={cache}"
        self.configure(option)
        entry, = cache.glob("*.log")
        entry.write_text(entry.read_text().replace("Result: 0", "Result: 1"))
        self.assertIn("ESBMC PASSED: cached", self.configure(option))

    def test_success_substring_is_not_a_verification_verdict(self):
        self.proof("apps", "result")
        checker = self.checker("print('diagnostic: VERIFICATION SUCCESSFUL was expected')")
        self.configure(executable=checker, success=False)

    def test_missing_find_is_reported_before_download(self):
        shim = self.root / "limited-path"
        shim.mkdir()
        for name in ("dirname", "uname", "curl", "unzip", "sha256sum", "flock", "realpath"):
            (shim / name).symlink_to(shutil.which(name))
        output = self.invoke(
            shutil.which("bash"), str(REPOSITORY / "scripts/install-esbmc.sh"),
            str(self.root / "installation"), success=False,
            env=dict(os.environ, PATH=str(shim)),
        )
        self.assertIn("requires find", output)
        self.assertNotIn("Downloading", output)

    def test_installer_reuses_existing_binary_without_downloading(self):
        destination = self.root / "installation"
        (destination / "bin").mkdir(parents=True)
        (destination / "bin/esbmc").symlink_to(self.esbmc)
        output = self.invoke("bash", str(REPOSITORY / "scripts/install-esbmc.sh"), str(destination))
        self.assertNotIn("Downloading", output)
        self.assertIn("ESBMC version", output)

    def test_bad_download_checksum_is_rejected_and_staging_cleaned(self):
        shim = self.root / "shim"
        shim.mkdir()
        curl = shim / "curl"
        curl.write_text(
            f"#!{sys.executable}\nimport sys\nfrom pathlib import Path\n"
            "Path(sys.argv[sys.argv.index('--output') + 1]).write_bytes(b'corrupt archive')\n"
        )
        curl.chmod(0o755)
        destination = self.root / "installation"
        output = self.invoke(
            "bash", str(REPOSITORY / "scripts/install-esbmc.sh"), str(destination),
            success=False, env=dict(os.environ, PATH=str(shim) + os.pathsep + os.environ["PATH"]),
        )
        self.assertIn("checksum mismatch", output)
        self.assertFalse(destination.exists())
        self.assertEqual(list(self.root.glob("installation.tmp.*")), [])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--esbmc", required=True, type=Path)
    parser.add_argument("--report-exception-modes", nargs="+", choices=("OFF", "ON"), default=["OFF"])
    args = parser.parse_args()
    GateIntegrationTests.report_exception_modes = args.report_exception_modes
    GateIntegrationTests.esbmc = args.esbmc.resolve()
    if not GateIntegrationTests.esbmc.is_file():
        parser.error("--esbmc must name an installed ESBMC executable")
    unittest.main(argv=[sys.argv[0]], verbosity=2)
