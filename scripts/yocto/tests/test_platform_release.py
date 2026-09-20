"""Exercise release preparation/publication and developer installation in isolation."""
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest
from unittest.mock import patch

SCRIPTS = Path(__file__).resolve().parents[1]


def load(name):
    spec = importlib.util.spec_from_file_location(name, SCRIPTS / f"{name}.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


publisher = load("publish-platform-release")
installer = load("install-platform-release")


class ReleaseTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.images = self.root / "yocto/build/tmp/deploy/images/qemuarm64"
        self.sdks = self.root / "yocto/build/tmp/deploy/sdk"
        self.images.mkdir(parents=True)
        self.sdks.mkdir()
        self.prefix = "adaptive-pi-image-qemuarm64.rootfs"
        self.data = dict(MACHINE="qemuarm64", IMAGE_BASENAME="adaptive-pi-image",
                         DISTRO_VERSION="5.0.20", SDK_VERSION="5.0.20",
                         SDK_NAME="poky-glibc-x86_64-adaptive-pi-image-cortexa57-qemuarm64",
                         TUNE_PKGARCH="cortexa57", SDKMACHINE="x86_64",
                         METADATA_REVISION=publisher.PIN)
        self.sdk = self.sdks / (self.data["SDK_NAME"] + "-toolchain-5.0.20.sh")
        self.sdk.write_text("fake SDK installer\n")
        self.sdk.with_suffix(".testdata.json").write_text(json.dumps(self.data))
        (self.images / f"{self.prefix}.testdata.json").write_text(json.dumps(self.data))
        (self.images / "kernel.bin").write_bytes(b"kernel")
        (self.images / "Image").symlink_to("kernel.bin")
        (self.images / f"{self.prefix}.ext4").write_bytes(b"rootfs")
        (self.images / f"{self.prefix}.manifest").write_text("package list\n")
        (self.images / f"{self.prefix}.qemuboot.conf").write_text(
            "[config_bsp]\nmachine=qemuarm64\nqb_default_fstype=ext4\n"
            "qb_system_name=qemu-system-aarch64\nimage_name=validated-build\n")
        runner = self.root / "scripts/qemu/run-platform.py"
        runner.parent.mkdir(parents=True)
        runner.write_text("print('launcher')\n")
        (self.root / "yocto/platform-version").write_text("v0.3.0\n")
        self.calls = []
        self.dirty = ""
        self.release = None
        self.tag_commit = "a" * 40
        self.auth_fail = False
        self.upload_fail = False

    def command(self, *args, cwd=None):
        self.calls.append(args)
        if args[0] == "sha256sum":
            return subprocess.check_output(args, cwd=cwd, text=True)
        if args[:3] == ("git", "rev-parse", "HEAD"):
            return publisher.PIN if str(cwd).endswith("poky") else "a" * 40
        if args[:3] == ("git", "status", "--porcelain"):
            return "" if str(cwd).endswith("poky") else self.dirty
        if args[:3] == ("gh", "auth", "status") and self.auth_fail:
            raise subprocess.CalledProcessError(1, args)
        if args[:3] == ("gh", "repo", "view"):
            return "owner/repo"
        if args[:2] == ("gh", "api"):
            endpoint = args[-1]
            if "releases?" in endpoint:
                return json.dumps([[self.release] if self.release else []])
            if "matching-refs" in endpoint:
                return json.dumps([{"ref": "refs/tags/v0.3.0", "object":
                                    {"type": "commit", "sha": self.tag_commit}}])
            if "/commits/" in endpoint:
                return json.dumps({"sha": "a" * 40})
        if args[:3] == ("gh", "release", "upload") and self.upload_fail:
            raise subprocess.CalledProcessError(1, args)
        return ""

    def publish(self, *extra):
        with patch.object(publisher, "ROOT", self.root), patch.object(publisher, "run", self.command), \
                patch.object(publisher.shutil, "which", return_value="gh"), \
                patch.object(sys, "argv", ["publish", "--release-version", "v0.3.0", *extra]), \
                contextlib.redirect_stdout(io.StringIO()):
            publisher.main()
        return self.root / "build/platform-releases/v0.3.0"

    def mutations(self):
        return [c for c in self.calls if c[:2] == ("gh", "release")]

    def test_prepare_portable_deterministic_assets(self):
        output = self.publish("--prepare-only")
        before = {p.name: p.read_bytes() for p in output.iterdir()}
        bundle = next(output.glob("*.tar.gz"))
        with tarfile.open(bundle) as archive:
            self.assertTrue(all(m.isfile() for m in archive.getmembers()))
            self.assertEqual(archive.extractfile("Image").read(), b"kernel")
        self.publish("--prepare-only")
        self.assertEqual(before, {p.name: p.read_bytes() for p in output.iterdir()})
        self.assertFalse(any(c[0] == "gh" for c in self.calls))
        self.assertEqual(len(before), 6)

    def test_missing_artifact_and_mismatched_metadata_fail_before_publication(self):
        (self.images / "Image").unlink()
        with self.assertRaises(ValueError):
            self.publish()
        self.assertFalse(self.mutations())
        (self.images / "Image").symlink_to("kernel.bin")
        self.data["SDK_VERSION"] = "wrong"
        self.sdk.with_suffix(".testdata.json").write_text(json.dumps(self.data))
        with self.assertRaisesRegex(ValueError, "SDK_VERSION"):
            self.publish()
        self.assertFalse(self.mutations())

    def test_dirty_tree_and_auth_fail_before_publication(self):
        self.dirty = " M yocto/platform-version"
        with self.assertRaisesRegex(ValueError, "Commit"):
            self.publish()
        self.auth_fail = True
        with self.assertRaises(subprocess.CalledProcessError):
            self.publish()
        self.assertFalse(self.mutations())

    def test_create_upload_then_publish(self):
        self.publish()
        self.assertEqual([c[2] for c in self.mutations()], ["create", "upload", "edit"])
        self.assertIn("--draft", self.mutations()[0])

    def test_existing_release_requires_deliberate_replace_and_same_commit(self):
        self.release = {"tag_name": "v0.3.0", "draft": False}
        with self.assertRaisesRegex(ValueError, "replace-assets"):
            self.publish()
        self.assertFalse(self.mutations())
        self.publish("--replace-assets")
        self.assertEqual([c[2] for c in self.mutations()], ["upload"])
        self.assertIn("--clobber", self.mutations()[0])
        self.calls.clear()
        self.tag_commit = "b" * 40
        with self.assertRaisesRegex(ValueError, "different commit"):
            self.publish("--replace-assets")
        self.assertFalse(self.mutations())

    def test_failed_upload_does_not_publish_draft(self):
        self.upload_fail = True
        with self.assertRaises(subprocess.CalledProcessError):
            self.publish()
        self.assertEqual([c[2] for c in self.mutations()], ["create", "upload"])

    def install(self, output, developer, commands):
        def command(args, **kwargs):
            commands.append(args)
            if args[0] == "sh":
                sdk = Path(args[-1])
                sdk.mkdir()
                (sdk / "environment-setup-cortexa57-poky-linux").write_text("environment")

        with patch.object(installer, "ROOT", developer), \
                patch.object(installer.shutil, "which", return_value="tool"), \
                patch.object(installer.urllib.request, "urlopen",
                             side_effect=lambda url, **kw: (output / url.rsplit("/", 1)[1]).open("rb")), \
                patch.object(installer.subprocess, "run", side_effect=command), \
                patch.object(sys, "argv", ["install", "--release-version", "v0.3.0"]), \
                contextlib.redirect_stdout(io.StringIO()):
            installer.main()

    def test_developer_install_and_no_overwrite(self):
        output = self.publish()
        developer = self.root / "developer"
        (developer / "yocto").mkdir(parents=True)
        commands = []
        self.install(output, developer, commands)
        self.assertTrue((developer / "yocto/platform/Image").is_file())
        self.assertEqual([c[0] for c in commands], ["sh", "cmake", "cmake"])
        with self.assertRaisesRegex(ValueError, "already exists"):
            self.install(output, developer, commands)
        self.assertEqual(len(commands), 3)

    def test_corrupted_download_never_executes_installer(self):
        output = self.publish()
        (output / self.sdk.name).write_text("corrupt")
        developer = self.root / "developer"
        (developer / "yocto").mkdir(parents=True)
        commands = []
        with self.assertRaisesRegex(ValueError, "Checksum mismatch"):
            self.install(output, developer, commands)
        self.assertFalse(commands)
        self.assertFalse((developer / "yocto/sdk").exists())


if __name__ == "__main__":
    unittest.main()
