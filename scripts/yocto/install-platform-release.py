#!/usr/bin/env python3
"""Download and install a versioned AdaptivePi platform into this repository."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request

ROOT = Path(__file__).resolve().parents[2]


def verify(path):
    checksum = path.with_name(path.name + ".sha256").read_text().split()
    if len(checksum) != 2 or checksum[1] != path.name:
        raise ValueError(f"Invalid checksum file for {path.name}")
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    if h.hexdigest() != checksum[0]:
        raise ValueError(f"Checksum mismatch for {path.name}")
    return h.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release-version", required=True, help="Explicit version, e.g. v0.3.0")
    parser.add_argument("--repo", default="Romanr92/adaptive-pi", help="GitHub owner/repository")
    args = parser.parse_args()
    version = args.release_version
    if not re.fullmatch(r"v(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)", version):
        raise ValueError("Expected vMAJOR.MINOR.PATCH")
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", args.repo):
        raise ValueError("Expected GitHub owner/repository")
    for tool in ("qemu-system-aarch64", "tmux", "ssh", "scp", "cmake", "ninja", "file", "readelf", "xz", "tar"):
        if not shutil.which(tool):
            raise ValueError(f"Install host prerequisite: {tool} (see publish-platform-release.md)")
    yocto = ROOT / "yocto"
    sdk = yocto / "sdk"
    platform = yocto / "platform"
    if sdk.exists() or platform.exists():
        raise ValueError("yocto/sdk or yocto/platform already exists; move it aside before installing")
    base = f"https://github.com/{args.repo}/releases/download/{version}"
    with tempfile.TemporaryDirectory(prefix=".platform-install-", dir=yocto) as temporary:
        stage = Path(temporary)

        def download(name):
            if Path(name).name != name or name in (".", ".."):
                raise ValueError("Unsafe release asset filename")
            path = stage / name
            print(f"Downloading {name}", flush=True)
            with urllib.request.urlopen(f"{base}/{name}", timeout=120) as response, path.open("wb") as output:
                shutil.copyfileobj(response, output)
            return path

        manifest_file = download("platform-manifest.json")
        download("platform-manifest.json.sha256")
        verify(manifest_file)
        manifest = json.loads(manifest_file.read_text())
        if (manifest["release_version"] != version or manifest["machine"] != "qemuarm64"
                or manifest["image"] != "adaptive-pi-image" or manifest["source_tree_dirty"]
                or manifest["sdk_host"] != "x86_64" or manifest["tune"] != "cortexa57"):
            raise ValueError("Release manifest has incompatible or uncommitted platform metadata")
        bundle = download(manifest["platform_bundle"])
        download(bundle.name + ".sha256")
        if verify(bundle) != manifest["platform_sha256"]:
            raise ValueError("Platform checksum differs from manifest")
        installer = download(manifest["sdk_installer"])
        download(installer.name + ".sha256")
        if verify(installer) != manifest["sdk_sha256"]:
            raise ValueError("SDK checksum differs from manifest")
        extracted = stage / "platform"
        extracted.mkdir()
        expected = {"Image", "adaptive-pi-image-qemuarm64.rootfs.ext4",
                    "adaptive-pi-image-qemuarm64.rootfs.qemuboot.conf",
                    "adaptive-pi-image-qemuarm64.rootfs.manifest", "run-platform.py"}
        with tarfile.open(bundle) as archive:
            members = archive.getmembers()
            if (len(members) != len(expected) or {m.name for m in members} != expected
                    or any(not m.isfile() or m.size <= 0 for m in members)):
                raise ValueError("Unexpected or unsafe platform archive contents")
            for member in members:
                with archive.extractfile(member) as source, (extracted / member.name).open("wb") as output:
                    shutil.copyfileobj(source, output)
        shutil.copyfile(manifest_file, extracted / manifest_file.name)
        # SDK paths are relocated by its installer; install directly at the final path.
        try:
            subprocess.run(["sh", str(installer), "-y", "-d", str(sdk)], check=True)
            if not (sdk / "environment-setup-cortexa57-poky-linux").is_file():
                raise ValueError("SDK installer did not produce an AArch64 environment")
            os.replace(extracted, platform)
        except Exception:
            if sdk.exists():
                shutil.rmtree(sdk)
            raise
    print(f"Installed platform {version}. Verifying the repository cross-build...", flush=True)
    subprocess.run(["cmake", "--preset", "debug-qemu-app"], cwd=ROOT, check=True)
    subprocess.run(["cmake", "--build", "--preset", "debug-qemu-app"], cwd=ROOT, check=True)
    print("Ready. Start with scripts/qemu/start-development-image.sh; deploy/debug as usual.")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, KeyError, OSError, tarfile.TarError, subprocess.CalledProcessError) as error:
        print(f"Platform installation failed: {error}", file=sys.stderr)
        sys.exit(1)
