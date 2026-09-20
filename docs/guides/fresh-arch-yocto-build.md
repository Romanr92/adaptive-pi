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

Use a 64-bit Arch Linux installation, either natively or through the
[WSL2 setup documented for this project](wsl-arch-yocto-host.md), with:

- at least 16 GiB RAM recommended;
- at least 100 GiB of free SSD space for the first build, downloads, and
  shared-state cache;
- network access while BitBake downloads sources;
- a normal user account with `sudo` access.

The first image build can take many hours. Run it in `tmux` and do not start
a second BitBake build at the same time.

## 2. Configure a UTF-8 locale

BitBake requires a UTF-8 locale. On a fresh Arch installation, enable
`en_US.UTF-8` before starting the build:

```bash
sudo sed -i 's/^#en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen
sudo locale-gen
echo 'LANG=en_US.UTF-8' | sudo tee /etc/locale.conf
export LANG=en_US.UTF-8
export LC_ALL=en_US.UTF-8
locale
```

`locale` must report a UTF-8 value for `LANG` and `LC_ALL`. Log out and
back in after this step so the configured locale is used by every new terminal.
Without it, BitBake fails before the server starts with an error such as
`Please make sure locale 'en_US.UTF-8' is available on your system`.

## 3. Install all required host tools

Update the system, then install the packages used by the Yocto build,
repository workflow, and QEMU development target:

```bash
sudo pacman -Syu --needed \
  base-devel \
  git \
  cmake \
  ninja \
  clang \
  clang-tools-extra \
  gtest \
  python \
  python-pexpect \
  python-gitpython \
  python-jinja \
  diffstat \
  chrpath \
  socat \
  cpio \
  rpcsvc-proto \
  xz \
  lz4 \
  bzip2 \
  gzip \
  tar \
  iputils \
  inetutils \
  xterm \
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
| `base-devel git python python-pexpect python-gitpython python-jinja diffstat chrpath socat cpio rpcsvc-proto xz lz4 bzip2 gzip tar iputils inetutils xterm file which unzip texinfo gawk wget zstd` | Poky/BitBake host build, fetch, helper scripts, and headless QEMU dependencies |
| `qemu-system-aarch64 tmux openssh` | Running, persisting, and accessing the QEMU target |
| `cmake ninja clang clang-tools-extra gtest` | Native AdaptivePi C++ builds, tests, formatting, and static analysis |

Verify the essential executables:

```bash
git --version
python --version
bitbake --version 2>/dev/null || true
qemu-system-aarch64 --version
locale -a | grep -Fx 'en_US.utf8'
tmux -V
cmake --version
ninja --version
```

`bitbake` is supplied by Poky, so it is expected to be unavailable until the
Poky environment is initialized.

## 4. Clone AdaptivePi and the pinned Poky source

These commands create every project directory that is not already provided by
the Git checkout. They do not assume a previous Yocto build, cache, SDK, or
QEMU image exists.

Choose a permanent workspace. The rest of this guide assumes
`~/workspace/adaptive-pi`.

```bash
mkdir -p ~/workspace
cd ~/workspace
git clone https://github.com/Romanr92/adaptive-pi.git
cd adaptive-pi
git status --short

git clone --branch scarthgap https://git.yoctoproject.org/poky yocto/poky
git -C yocto/poky checkout 77d1feb37e280733684ae8a9449fb031d5d7ff40
git -C yocto/poky status --short
```

`git status --short` and the final Poky status command must print no output. `yocto/poky/` is intentionally ignored
by Git: it is external upstream metadata, pinned by
[`yocto/README.md`](../../yocto/README.md).

Do not place `yocto/build/`, `yocto/downloads/`,
`yocto/sstate-cache/`, or `yocto/sdk/` under version control. They are
machine-local generated data and are already ignored.

## 5. Start a persistent build shell

Use `tmux` so a terminal or SSH disconnect does not interrupt the build:

```bash
tmux new-session -s adaptive-pi-yocto
```

Inside tmux, initialize a new build directory:

```bash
cd ~/workspace/adaptive-pi
repo_root="$(git rev-parse --show-toplevel)"
source "$repo_root/yocto/poky/oe-init-build-env" "$repo_root/yocto/build"
```

`repo_root` is the absolute path of the checked-out AdaptivePi repository.
`$PWD` is a shell variable that means “the current working directory”; it
changes to `$repo_root/yocto/build` when `oe-init-build-env` finishes.
Using `repo_root` keeps later paths unambiguous.

This command makes `bitbake` and `bitbake-layers` available. It does not
start a build.

Detach without stopping the build with `Ctrl+B`, then `D`. Reattach later
with:

```bash
tmux attach -t adaptive-pi-yocto
```

## 6. Apply the AdaptivePi build configuration

### Choose host-specific settings first

The committed configuration uses two tasks and two compiler jobs because it was
created on a constrained host. Before registering the layer, choose local
overrides for a more powerful machine. These overrides are machine-local and
must not be committed.

For an **i9-14900KF with 64 GiB RAM** running WSL2 with a 48 GiB memory cap and
24 WSL processors, begin with:

```conf
BB_NUMBER_THREADS = "20"
PARALLEL_MAKE = "-j 20"
```

For an **ASUS TUF A18 FA808U** using the 32 GiB-RAM WSL profile from the WSL
guide (16 GiB WSL memory and four processors), begin with:

```conf
BB_NUMBER_THREADS = "4"
PARALLEL_MAKE = "-j 4"
```

After the project block is appended below, add the matching two lines to
`conf/local.conf`. For other systems, start with one job per 4 GiB of available
WSL/Linux memory, capped at the configured logical CPU count. Lower the values
if the host starts swapping.

### Repair an existing malformed configuration before BitBake starts

A fresh build does not need this repair. If a previous append joined two
settings, repair it **before** running `bitbake-layers`, because BitBake must
parse `conf/local.conf` before it can add a layer:

```bash
nano conf/local.conf
```

Find a line resembling:

```conf
PREFERRED_PROVIDER_virtual/kernel = "linux-yocto-rt"# AdaptivePi Release 1: generic 64-bit ARM QEMU target.
```

Split it into:

```conf
PREFERRED_PROVIDER_virtual/kernel = "linux-yocto-rt"
# AdaptivePi Release 1: generic 64-bit ARM QEMU target.
```

Remove any duplicate AdaptivePi configuration block, save with `Ctrl+O`, press
Enter, and exit with `Ctrl+X`.

Still in the initialized build shell, register the committed project layer and
append the repository's baseline configuration. The commands are safe to rerun:
they do not add the layer or configuration block twice.

```bash
layer_path="$repo_root/yocto/meta-adaptive-pi"
config_marker='# AdaptivePi Release 1: generic 64-bit ARM QEMU target.'

if ! grep -Fqx "$layer_path" conf/bblayers.conf; then
  bitbake-layers add-layer "$layer_path"
fi

if ! grep -Fqx "$config_marker" conf/local.conf; then
  printf '\n' >> conf/local.conf
  cat "$repo_root/yocto/config/local.conf.append" >> conf/local.conf
  printf '\n' >> conf/local.conf
fi

bitbake-layers show-layers
tail -n 20 conf/local.conf
```

The leading and trailing `printf '\n'` commands ensure the copied block is
always separated from neighbouring settings, even when a configuration file
lacks a final newline.

The committed baseline selects:

- `MACHINE = "qemuarm64"`;
- persistent `yocto/downloads/` and `yocto/sstate-cache/` directories;
- two BitBake tasks and compiler jobs, the conservative setting used by the
  original AdaptivePi host;
- `systemd`;
- the `linux-yocto-rt` PREEMPT_RT kernel provider.

These commands are for a new `yocto/build/` directory, and are safe to rerun.
Use the committed configuration as the source of truth.

## 7. Build the image

First prove that BitBake resolves the project configuration:

```bash
bitbake-layers show-recipes linux-yocto-rt
bitbake -e virtual/kernel | grep -E '^(PN|PREFERRED_PROVIDER_virtual/kernel)='
```

The resolved kernel provider must be `linux-yocto-rt`.

Build order for this project:

- **Normal path — build `adaptive-pi-image` directly.** BitBake automatically
  builds every dependency, including the kernel and the minimal-image base. This
  is the required reproducible AdaptivePi image.
- **Optional diagnostic path — build `core-image-minimal` first.** Use this
  only when bringing up a new host or investigating a generic Poky/QEMU failure.
  It proves the upstream baseline before project metadata is involved, but is not
  required before the AdaptivePi image.

For the normal path, start the complete AdaptivePi image build:

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

## 8. Boot and verify the QEMU image

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

## 9. Create the SDK for application development

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
