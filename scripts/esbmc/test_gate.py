#!/usr/bin/env python3
"""Exercise issue #42's gate with real proofs and isolated failure fixtures.

No network or production-source edits: each test owns a temporary CMake project.
The real solver checks passing/failing assertions; test doubles exercise process
errors and timeouts. Run with --esbmc /absolute/path/to/the/pinned/esbmc.
"""

import argparse
import os
from pathlib import Path
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
        self.assertIn("conflicts with the required Z3 solver", self.configure(success=False))
        checker = self.checker("sys.exit(0)", solvers="bitwuzla")
        self.assertIn("must provide the Z3 solver", self.configure(executable=checker, success=False))

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
    args = parser.parse_args()
    GateIntegrationTests.esbmc = args.esbmc.resolve()
    if not GateIntegrationTests.esbmc.is_file():
        parser.error("--esbmc must name an installed ESBMC executable")
    unittest.main(argv=[sys.argv[0]], verbosity=2)
