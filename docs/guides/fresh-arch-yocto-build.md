# Start an AdaptivePi Yocto build from a fresh Arch Linux installation

This guide gets a new developer from a fresh Arch Linux installation to the
current AdaptivePi `qemuarm64` development image.

It builds the repository's pinned **Poky Scarthgap 5.0.20** baseline,
`adaptive-pi-image`, with **systemd** and the `linux-yocto-rt`
PREEMPT_RT kernel. The result runs in generic AArch64 QEMU; it is not a
Raspberry Pi emulator.

> Arch Linux is a rolling-release host and is not one of Yocto's primary
> validated host distributions. These steps are the tested AdaptivePi setup.
> Keep the package set and Poky commit pinned when reproducing a build.

## 1. Host requirements

Use a native 64-bit Arch Linux installation with:

- at least 16 GiB RAM recommended;
- at least 100 GiB of free SSD space for the first build, downloads, and
  shared-state cache;
- network access while BitBake downloads sources;
- a normal user account with `sudo` access.

The first image build can take many hours. Run it in `tmux` and do not start
a second BitBake build at the same time.

## 2. Install all required host tools

Update the system, then install the packages used by the Yocto build,
repository workflow, and QEMU development target:

```bash
sudo pacman -Syu --needed \
  base-devel \
  git \
  cmake \
  ninja \
  clang \
  clang-tools \
  gtest \
  python \
  python-pexpect \
  diffstat \
  chrpath \
  cpio \
  rpcsvc-proto \
  xz \
  file \
  which \
  unzip \
  texinfo \
  gawk \
  wget \
  zstd \
  qemu-system-aarch64 \
  tmux \
  openssh
```

Tool groups:

| Tools | Used for |
|---|---|
| `base-devel git python python-pexpect diffstat chrpath cpio rpcsvc-proto xz file which unzip texinfo gawk wget zstd` | Poky/BitBake host build and fetch dependencies |
| `qemu-system-aarch64 tmux openssh` | Running, persisting, and accessing the QEMU target |
| `cmake ninja clang clang-tools gtest` | Native AdaptivePi C++ builds, tests, formatting, and static analysis |

Verify the essential executables:

```bash
git --version
python --version
bitbake --version 2>/dev/null || true
qemu-system-aarch64 --version
tmux -V
cmake --version
ninja --version
```

`bitbake` is supplied by Poky, so it is expected to be unavailable until the
Poky environment is initialized.

## 3. Clone AdaptivePi and the pinned Poky source

Choose a permanent workspace. The rest of this guide assumes
`~/workspace/adaptive-pi`.

```bash
mkdir -p ~/workspace
cd ~/workspace
git clone https://github.com/Romanr92/adaptive-pi.git
cd adaptive-pi

git clone --branch scarthgap https://git.yoctoproject.org/poky yocto/poky
git -C yocto/poky checkout 77d1feb37e280733684ae8a9449fb031d5d7ff40
git -C yocto/poky status --short
```

The last command must print no output. `yocto/poky/` is intentionally ignored
by Git: it is external upstream metadata, pinned by
[`yocto/README.md`](../../yocto/README.md).

Do not place `yocto/build/`, `yocto/downloads/`,
`yocto/sstate-cache/`, or `yocto/sdk/` under version control. They are
machine-local generated data and are already ignored.

## 4. Start a persistent build shell

Use `tmux` so a terminal or SSH disconnect does not interrupt the build:

```bash
tmux new-session -s adaptive-pi-yocto
```

Inside tmux, initialize a new build directory:

```bash
cd ~/workspace/adaptive-pi
source yocto/poky/oe-init-build-env "$PWD/yocto/build"
```

This command changes the current directory to `yocto/build/` and makes
`bitbake` and `bitbake-layers` available. It does not start a build.

Detach without stopping the build with `Ctrl+B`, then `D`. Reattach later
with:

```bash
tmux attach -t adaptive-pi-yocto
```

## 5. Apply the AdaptivePi build configuration

Still in the initialized build shell, register the committed project layer and
append the repository's host-safe configuration:

```bash
bitbake-layers add-layer "$PWD/../meta-adaptive-pi"
cat "$PWD/../config/local.conf.append" >> conf/local.conf

bitbake-layers show-layers
tail -n 20 conf/local.conf
```

The configuration selects:

- `MACHINE = "qemuarm64"`;
- persistent `yocto/downloads/` and `yocto/sstate-cache/` directories;
- at most two BitBake tasks and compiler jobs, matching the current resource
  policy;
- `systemd`;
- the `linux-yocto-rt` PREEMPT_RT kernel provider.

These commands are for a new `yocto/build/` directory. Do not append
`local.conf.append` again to an existing build directory; use the committed
configuration as the source of truth and update the existing file deliberately.

## 6. Build the image

First prove that BitBake resolves the project configuration:

```bash
bitbake-layers show-recipes linux-yocto-rt
bitbake -e virtual/kernel | grep -E '^(PN|PREFERRED_PROVIDER_virtual/kernel)='
```

The resolved kernel provider must be `linux-yocto-rt`.

Start the complete AdaptivePi image build:

```bash
bitbake adaptive-pi-image
```

Important rules while it runs:

- Keep the tmux session open or detached; do not terminate it.
- Preserve `yocto/downloads/` and `yocto/sstate-cache/` for future
  incremental builds.
- Do not use `bitbake -c cleanall` unless you understand the exact recovery
  need; it discards reusable downloads and build state.
- Do not modify Yocto metadata during the build. Commit changes before starting
  a reproducible image build.

The build artifacts are written to:

```text
yocto/build/tmp/deploy/images/qemuarm64/
```

The expected AdaptivePi boot configuration is:

```text
adaptive-pi-image-qemuarm64.rootfs.qemuboot.conf
```

## 7. Boot and verify the QEMU image

From another terminal at the repository root, start the managed QEMU target:

```bash
cd ~/workspace/adaptive-pi
scripts/qemu/start-development-image.sh
```

It starts QEMU in its own `adaptive-pi-qemu` tmux session and waits for SSH.
Connect to the guest:

```bash
ssh -p 2222 root@localhost
```

In the guest, verify the platform:

```bash
uname -a
cat /sys/kernel/realtime
systemctl --version
```

Expected evidence:

- `uname -a` reports an AArch64 PREEMPT_RT kernel;
- `/sys/kernel/realtime` prints `1`;
- `systemctl` is available.

Leave the guest with `exit`, then stop it cleanly on the host:

```bash
scripts/qemu/stop-development-image.sh
```

Use `scripts/qemu/stop-development-image.sh --force` only when a clean
shutdown cannot complete.

## 8. Create the SDK for application development

The image is rebuilt only for target-image, kernel, package, or system-service
changes. For ordinary C++ application work, generate and use the matching SDK:

```bash
tmux attach -t adaptive-pi-yocto
bitbake adaptive-pi-image -c populate_sdk
```

Install the generated installer into the ignored `yocto/sdk/` directory:

```bash
cd ~/workspace/adaptive-pi
sh yocto/build/tmp/deploy/sdk/poky-glibc-*-adaptive-pi-image-*-toolchain-5.0.20.sh \
  -d "$PWD/yocto/sdk"
```

In every new terminal used for an ARM64 cross-build, source:

```bash
source yocto/sdk/environment-setup-cortexa57-poky-linux
```

Continue with the [SDK cross-build, QEMU deployment, and debugging guide](sdk-cross-build-and-qemu-deployment.md).

## Troubleshooting

| Symptom | Check |
|---|---|
| `bitbake: command not found` | Re-run `source yocto/poky/oe-init-build-env "$PWD/yocto/build"` from the repository root. |
| Poky checkout reports local changes | Ensure the checkout is at `77d1feb37e280733684ae8a9449fb031d5d7ff40`; do not modify upstream Poky files for AdaptivePi changes. |
| Build fails due to missing host command/module | Re-run the package installation command in section 2, then retry the failed task. |
| Host becomes unresponsive or swaps heavily | Keep the committed `BB_NUMBER_THREADS = "2"` and `PARALLEL_MAKE = "-j 2"` limits; close other heavy workloads. |
| QEMU script cannot find a boot configuration | Complete `bitbake adaptive-pi-image` successfully and check `yocto/build/tmp/deploy/images/qemuarm64/`. |
| SSH to port 2222 fails | Inspect the serial console with `tmux attach -t adaptive-pi-qemu`; do not start a second QEMU instance. |

For design rationale and the detailed image composition, see the
[Yocto QEMU PREEMPT_RT build guide](yocto-qemu-preempt-rt-build-guide.md).
