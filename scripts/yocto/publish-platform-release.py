#!/usr/bin/env python3
"""Manually package and publish the validated qemuarm64 platform."""
import argparse
import configparser
import gzip
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

ROOT = Path(__file__).resolve().parents[2]
IMAGE = "adaptive-pi-image"
MACHINE = "qemuarm64"
PIN = "77d1feb37e280733684ae8a9449fb031d5d7ff40"


def run(*args, cwd=ROOT):
    return subprocess.check_output(args, cwd=cwd, text=True).strip()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def nonempty(path):
    require(path.is_file() and path.stat().st_size > 0, f"Missing or empty artifact: {path}")
    return path


def metadata(image_data, sdk_data):
    for key in ("MACHINE", "IMAGE_BASENAME", "DISTRO_VERSION", "SDK_VERSION",
                "SDK_NAME", "SDKMACHINE", "TUNE_PKGARCH", "METADATA_REVISION"):
        require(image_data.get(key) and image_data[key] == sdk_data.get(key),
                f"Image/SDK metadata mismatch: {key}")
    require(image_data["SDKMACHINE"] == "x86_64" and image_data["TUNE_PKGARCH"] == "cortexa57",
            "Repository presets require the x86_64 SDK for cortexa57")
    require(image_data["MACHINE"] == MACHINE, "Expected qemuarm64")
    require(image_data["IMAGE_BASENAME"] == IMAGE, "Expected adaptive-pi-image")
    require(image_data["METADATA_REVISION"] == PIN, "Build does not use the pinned Poky commit")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release-version", required=True, help="Explicit tag, e.g. v0.3.0")
    parser.add_argument("--prepare-only", action="store_true", help="Package locally; never contact GitHub")
    parser.add_argument("--replace-assets", action="store_true", help="Deliberately replace assets on this release")
    args = parser.parse_args()
    version = args.release_version
    require(re.fullmatch(r"v(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)", version),
            "Use an explicit stable version: vMAJOR.MINOR.PATCH")
    deploy = ROOT / "yocto/build/tmp/deploy"
    images = deploy / "images" / MACHINE
    prefix = f"{IMAGE}-{MACHINE}.rootfs"
    image_data = json.loads(nonempty(images / f"{prefix}.testdata.json").read_text())
    sdk_name = f"{image_data['SDK_NAME']}-toolchain-{image_data['SDK_VERSION']}"
    require(Path(sdk_name).name == sdk_name, "Invalid SDK filename in metadata")
    sdk = nonempty(deploy / "sdk" / f"{sdk_name}.sh")
    sdk_data = json.loads(nonempty(sdk.with_suffix(".testdata.json")).read_text())
    metadata(image_data, sdk_data)
    poky = ROOT / "yocto/poky"
    require(run("git", "rev-parse", "HEAD", cwd=poky) == PIN, "Poky checkout differs from pinned commit")
    require(not run("git", "status", "--porcelain", cwd=poky), "Poky checkout must be clean")
    files = [nonempty(images / name) for name in (
        "Image", f"{prefix}.ext4", f"{prefix}.qemuboot.conf", f"{prefix}.manifest")]
    config = configparser.ConfigParser(interpolation=None)
    config.read(files[2])
    require(config["config_bsp"]["machine"] == MACHINE, "QEMU machine mismatch")
    require(config["config_bsp"]["qb_default_fstype"] == "ext4", "Expected ext4 rootfs")
    require(config["config_bsp"]["qb_system_name"] == "qemu-system-aarch64", "Expected AArch64 QEMU")
    if sdk.with_suffix(".sh.sha256").exists():
        run("sha256sum", "-c", sdk.name + ".sha256", cwd=sdk.parent)

    # Make the version change reviewable and committed before any remote mutation.
    version_file = ROOT / "yocto/platform-version"
    previous = version_file.read_text().strip() if version_file.exists() else None
    if not args.prepare_only:
        require(shutil.which("gh"), "GitHub CLI (gh) is required; install it and run gh auth login")
        run("gh", "auth", "status")
    if previous != version:
        version_file.write_text(version + "\n")
    dirty = bool(run("git", "status", "--porcelain"))
    require(args.prepare_only or not dirty,
            "Commit the platform-version update and all release source changes, then rerun; nothing published")
    commit = run("git", "rev-parse", "HEAD")
    repo = None
    release = None
    if not args.prepare_only:
        repo = run("gh", "repo", "view", "--json", "nameWithOwner", "--jq", ".nameWithOwner")
        # Distinguish a missing release from API/auth/network failures using a successful listing.
        releases = json.loads(run("gh", "api", "--paginate", "--slurp", f"repos/{repo}/releases?per_page=100"))
        release = next((r for page in releases for r in page if r["tag_name"] == version), None)
        refs = json.loads(run("gh", "api", f"repos/{repo}/git/matching-refs/tags/{version}"))
        ref = next((r for r in refs if r["ref"] == f"refs/tags/{version}"), None)
        if ref:
            obj = ref["object"]
            while obj["type"] == "tag":
                obj = json.loads(run("gh", "api", f"repos/{repo}/git/tags/{obj['sha']}"))["object"]
            require(obj["type"] == "commit" and obj["sha"] == commit, "Remote tag points to a different commit")
        require(not release or ref, "Existing release must have a matching remote tag")
        require(not release or args.replace_assets, "Release exists; use --replace-assets to update deliberately")
        # HEAD must already exist on GitHub, even when creating a new tag.
        remote_commit = json.loads(run("gh", "api", f"repos/{repo}/commits/{commit}"))
        require(remote_commit["sha"] == commit, "Push the release commit before publishing")

    output_parent = ROOT / "build/platform-releases"
    output_parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=f".{version}-", dir=output_parent) as temporary:
        stage = Path(temporary)
        bundle = stage / f"adaptive-pi-platform-{version}-{MACHINE}.tar.gz"
        # Stable archive headers allow an identical rerun to produce identical checksums.
        with bundle.open("wb") as raw, gzip.GzipFile(filename="", fileobj=raw, mode="wb", mtime=0) as zipped:
            with tarfile.open(fileobj=zipped, mode="w", dereference=True) as archive:
                for path in files + [ROOT / "scripts/qemu/run-platform.py"]:
                    info = archive.gettarinfo(str(path), arcname=path.name)
                    info.uid = info.gid = info.mtime = 0
                    info.uname = info.gname = ""
                    with path.open("rb") as stream:
                        archive.addfile(info, stream)
        staged_sdk = stage / sdk.name
        shutil.copyfile(sdk, staged_sdk)
        manifest = {
            "release_version": version, "adaptive_pi_commit": commit,
            "source_tree_dirty": dirty, "poky_commit": PIN,
            "yocto_version": image_data["DISTRO_VERSION"], "machine": MACHINE,
            "image": IMAGE, "sdk_installer": sdk.name, "sdk_sha256": digest(staged_sdk),
            "platform_bundle": bundle.name, "platform_sha256": digest(bundle),
            "sdk_version": image_data["SDK_VERSION"],
            "sdk_host": image_data["SDKMACHINE"], "tune": image_data["TUNE_PKGARCH"],
            "image_build": config["config_bsp"]["image_name"],
        }
        manifest_file = stage / "platform-manifest.json"
        manifest_file.write_text(json.dumps(manifest, indent=2) + "\n")
        assets = [bundle, staged_sdk, manifest_file]
        for asset in assets.copy():
            checksum = stage / (asset.name + ".sha256")
            checksum.write_text(f"{digest(asset)}  {asset.name}\n")
            assets.append(checksum)
            run("sha256sum", "-c", checksum.name, cwd=stage)
        destination = output_parent / version
        # Only replace our known output files, preserving unrelated files.
        destination.mkdir(exist_ok=True)
        for asset in assets:
            os.replace(asset, destination / asset.name)
    print(f"Verified release assets: {destination}", flush=True)
    if args.prepare_only:
        print("Local preparation only. Commit the version/source changes and push before publishing.")
        return
    paths = [str(destination / asset.name) for asset in assets]
    if not release:
        run("gh", "release", "create", version, "--repo", repo, "--target", commit,
            "--draft", "--title", f"AdaptivePi platform {version}", "--notes",
            f"Validated qemuarm64 platform and matching SDK. Source: {commit}. See platform-manifest.json.")
    upload = ["gh", "release", "upload", version, "--repo", repo, *paths]
    if args.replace_assets:
        upload.append("--clobber")
    run(*upload)
    if not release or release["draft"]:
        run("gh", "release", "edit", version, "--repo", repo, "--draft=false")
    print(f"Published https://github.com/{repo}/releases/tag/{version}")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, KeyError, OSError, configparser.Error, subprocess.CalledProcessError) as error:
        print(f"Release failed: {error}", file=sys.stderr)
        sys.exit(1)
