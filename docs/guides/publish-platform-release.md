# Publish and install a versioned Yocto platform

Issue [#46](https://github.com/Romanr92/adaptive-pi/issues/46) adds a manual
post-build release workflow. It publishes a QEMU kernel/root filesystem bundle
and its matching SDK. No Yocto build runs in CI or during developer installation.

## Maintainer: prepare and publish

Use a Linux host with Python 3, Git, `sha256sum`, and an authenticated GitHub CLI
(`gh auth login`) with permission to publish releases. Start from a successful,
boot-tested `adaptive-pi-image` build using the pinned Poky checkout in
[yocto/README.md](../../yocto/README.md). Generate the matching SDK in that same
initialized build environment:

```bash
bitbake adaptive-pi-image -c populate_sdk
```

From the repository root, explicitly choose a version:

```bash
scripts/yocto/publish-platform-release.sh --release-version v0.3.0 --prepare-only
```

This validates the image and SDK metadata, updates `yocto/platform-version`, and
assembles assets in `build/platform-releases/v0.3.0/`. This file is the
project-owned platform release version; it is separate from Poky's SDK version
(e.g. `5.0.20`) and from the application release roadmap. The initial value is
`v0.2.0`, matching the existing SDK CI reference. Preparation performs no network
access and does not require GitHub CLI. A manifest prepared with uncommitted
changes is explicitly marked and must not be distributed as a release.

Review and commit the source changes and `yocto/platform-version`, then push
that commit. Run the publisher again:

```bash
scripts/yocto/publish-platform-release.sh --release-version v0.3.0
```

Publishing requires a clean repository and clean pinned Poky checkout. If the
requested version differs, the script updates the version file and stops before
publishing so that the change can be committed. The final manifest records the
committed version's exact AdaptivePi SHA. The commit must already exist on GitHub.
An existing tag must resolve to that same commit; the script never moves tags.

The publisher creates a draft release, uploads all six assets, and publishes
only after every upload succeeds:

- `adaptive-pi-platform-v0.3.0-qemuarm64.tar.gz` and `.sha256`;
- the Yocto SDK `.sh` installer and `.sha256`;
- `platform-manifest.json` and `.sha256`.

The bundle contains regular files for the kernel, ext4 root filesystem, QEMU
configuration, image package manifest, and portable QEMU launcher. Deploy
symlinks are dereferenced, so it has no dependency on the maintainer's build
paths. The launcher uses host `qemu-system-aarch64` and the bundled QEMU options;
it does not need Poky's `runqemu` or its native sysroot.

The release manifest records the release version, AdaptivePi commit, pinned
Poky commit, Poky/SDK versions, image build name, machine, SDK host/tuning, and
bundle/SDK filenames and SHA-256 digests. Image and SDK testdata must agree on
platform metadata. This checks compatibility, not runtime correctness: the
maintainer is responsible for validating the image and generating its matching
SDK from the same build. Checksum sidecars are generated and verified before
any release mutation; an existing SDK sidecar is also checked if present.

## Retry or deliberately replace assets

A normal rerun refuses to modify an existing release. To retry an interrupted
upload or deliberately replace assets for the same tag and source commit:

```bash
scripts/yocto/publish-platform-release.sh --release-version v0.3.0 --replace-assets
```

This updates the same release with `gh release upload --clobber`. Initial upload
failures leave a draft. GitHub cannot atomically replace all assets on an already
published release: a failed replacement can leave mixed assets until a successful
retry. Downloaders reject inconsistent checksums. Prefer a new version for a
changed platform; ordinary developer installs should consume immutable releases.
The script preserves unrelated release assets and release notes.

After publishing, update the SDK release and installer reference in
`.github/workflows/host-ci.yml` when CI should adopt the new platform. Publishing
does not silently change CI's selected SDK.

## Developer: install without rebuilding Yocto

Use an x86_64 Linux host with Python 3, Git, QEMU's AArch64 system emulator,
`tmux`, OpenSSH client (including `scp`), CMake 3.25+, Ninja, `file`, binutils, `tar`, and `xz`.
The SDK supplies the cross-compiler and cross-GDB. For example, on Arch Linux:

```bash
sudo pacman -S --needed python git qemu-system-aarch64 tmux openssh cmake ninja file binutils tar xz
```

Clone the repository and run:

```bash
scripts/yocto/install-platform-release.sh --release-version v0.3.0
```

The script downloads public GitHub release assets over HTTPS (no GitHub login
required), verifies all checksums and manifest compatibility before executing the
SDK installer, safely extracts the bundle, installs the SDK at `yocto/sdk`, and
runs the existing `debug-qemu-app` configure/build presets. An optional
`--repo owner/repository` selects a fork's releases. Only install releases from a
repository you trust: the SDK installer and bundled launcher are executable code.

The downloaded platform lives in ignored `yocto/platform/`. The installed SDK
and image are local development data, never committed. The installer refuses to
overwrite an existing `yocto/sdk` or `yocto/platform`; stop QEMU and move those
directories aside before installing a different release. If an SDK install
fails, its partial new SDK directory is removed. If the cross-build fails, the
installed platform remains available so the reported build problem can be fixed
and the CMake commands rerun. A stale `build/debug-qemu-app` CMake cache from
another SDK may need to be moved aside before configuring.

Start, deploy, and debug with the existing workflow:

```bash
scripts/qemu/start-development-image.sh
scripts/qemu/deploy-binary.sh build/debug-qemu-app/apps/hello-adaptive/hello-adaptive
ssh -p 2222 root@localhost /usr/local/bin/hello-adaptive
scripts/qemu/stop-development-image.sh
```

The managed start script prefers the installed release when present and otherwise
uses the local Yocto build. It retains the same tmux session, SSH readiness check,
and localhost SSH port 2222. Guest filesystem changes persist in the extracted
ext4 image; retain the release asset if you need a pristine reset. VS Code's
existing QEMU debug configuration uses the installed SDK as before.

## Validation

Run the isolated release workflow tests (temporary repositories and mocked GitHub,
SDK installer, and downloads; no actual publication):

```bash
python3 -m unittest discover -s scripts/yocto/tests -v
```

For local artifact validation, run `--prepare-only` with the intended version.
Actual publication still requires a validated image and a committed, pushed
release source tree. The implementation does not automatically publish a release
as part of testing.

Local verification for this change used the existing Poky 5.0.20 artifacts:
preparation and checksum verification passed, the extracted bundle booted to
login under host QEMU using a temporary snapshot, and the real SDK installed
and cross-built `hello-adaptive` in a fresh temporary repository. The install
smoke test served local assets in place of GitHub downloads. GitHub publication
was exercised with mocks only; no release was published during implementation.
