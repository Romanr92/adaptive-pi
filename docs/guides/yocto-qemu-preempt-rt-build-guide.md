# AdaptivePi Yocto QEMU PREEMPT_RT Build Guide

This guide records the first successful AdaptivePi embedded-Linux workflow:

```text
Arch Linux host -> Yocto/Poky cross-build -> qemuarm64 guest -> AdaptivePi image
```

The resulting guest is **Poky 5.0.20 on AArch64**, not Arch Linux and not a faithful Raspberry Pi emulator. It is a fast ARM64 integration environment for systemd services, C++ applications, SSH, debugging, and later Adaptive-style middleware work.

## 1. Architecture and scope

| Layer | Role | Why it is separate |
|---|---|---|
| Windows 11 | VS Code user interface | Provides the desktop UI. |
| Arch Linux | Source checkout, Yocto build host, QEMU host | Runs native tools and long-lived builds. |
| Yocto / Poky | Cross-build system and reference metadata | Creates the complete target Linux image from recipes. |
| QEMU `qemuarm64` | Generic ARM64 virtual target | Validates boot and software integration quickly. |
| Raspberry Pi, later | Real hardware target | Required for board peripherals and real latency evidence. |

QEMU does not faithfully emulate Raspberry Pi firmware, GPIO, I2C, SPI, Wi-Fi, Bluetooth, GPU/display, or hardware timing. Those are Release 2-and-later hardware-validation concerns.

## 2. Persistent Yocto shell

Run long builds inside tmux so a VS Code or SSH reconnect does not stop them.

```bash
tmux new-session -s adaptive-pi-yocto bash
```

Initialize the existing build environment:

```bash
source ~/workspace/adaptive-pi/yocto/poky/oe-init-build-env \
  ~/workspace/adaptive-pi/yocto/build
```

`oe-init-build-env` is provided by Poky. It exports the BitBake environment and changes into the configured build directory. It does not rebuild anything.

Useful tmux commands:

```text
Detach: Ctrl+B, then D
Reattach: tmux attach -t adaptive-pi-yocto
```

## 3. Build and boot the minimal baseline

The first build deliberately used the smallest official bootable reference image:

```bash
bitbake core-image-minimal
```

Why this comes first:

- It proves the host, cross-toolchain, source downloads, and ARM64 image pipeline before AdaptivePi-specific changes are introduced.
- It keeps the image small, so every later package is an explicit design decision.
- It creates local `downloads` and `sstate-cache` data that make later builds incremental.

The important generated artifacts are in:

```text
yocto/build/tmp/deploy/images/qemuarm64/
```

Examples:

- `Image-qemuarm64.bin`: ARM64 Linux kernel.
- `core-image-minimal-qemuarm64.rootfs.ext4`: target root filesystem.
- `*.qemuboot.conf`: QEMU boot configuration.

Install the required system emulator on Arch if it is missing:

```bash
sudo pacman -S --needed qemu-system-aarch64
qemu-system-aarch64 --version
```

Boot through the serial console:

```bash
runqemu qemuarm64 nographic
```

At the login prompt, use `root` without a password. Validate the guest:

```bash
uname -a
cat /etc/issue
```

Expected facts:

- architecture: `aarch64`;
- distribution: `Poky (Yocto Project Reference Distro) 5.0.20`.

`core-image-minimal` may not include `/etc/os-release`; that is normal for this deliberately small image.

Exit QEMU's serial console with `Ctrl+A`, then `X`.

## 4. Own the product metadata with a custom layer

Do not edit Poky recipes directly. AdaptivePi configuration belongs in a project-owned layer.

```bash
bitbake-layers create-layer ~/workspace/adaptive-pi/yocto/meta-adaptive-pi
bitbake-layers add-layer ~/workspace/adaptive-pi/yocto/meta-adaptive-pi
bitbake-layers show-layers
```

`create-layer` creates the metadata skeleton. `add-layer` adds it to the active build's generated `conf/bblayers.conf`, allowing BitBake to find it.

The layer structure is:

```text
yocto/meta-adaptive-pi/
  conf/layer.conf
  recipes-core/images/adaptive-pi-image.bb
  README
```

The layer is committed to Git. The generated `yocto/build/`, `downloads/`, and `sstate-cache/` directories remain machine-local and must not be committed.

## 5. Define the development image

`yocto/meta-adaptive-pi/recipes-core/images/adaptive-pi-image.bb`:

```bitbake
SUMMARY = "AdaptivePi development image for generic AArch64 QEMU"
DESCRIPTION = "Minimal AdaptivePi target image with SSH, C++ runtime, and debugging support."
LICENSE = "Apache-2.0"

require recipes-core/images/core-image-minimal.bb

IMAGE_FEATURES += " ssh-server-openssh"

IMAGE_INSTALL:append = " \
    gdbserver \
    libstdc++ \
"
```

Why these packages are included:

| Item | Reason |
|---|---|
| `core-image-minimal` | Reuses the already proven minimal image definition. |
| `ssh-server-openssh` | Enables future remote login and deployment into QEMU. |
| `gdbserver` | Enables host-controlled debugging of ARM64 target processes. |
| `libstdc++` | Supplies the target runtime for C++ application binaries. |

Alternatives:

- **Dropbear** is a smaller SSH server; OpenSSH is preferred for a development image.
- **core-image-full-cmdline** includes more utilities but reduces visibility into image dependencies.
- A future production image should remove debugging tools and include only strictly necessary packages.

## 6. Select systemd and the PREEMPT_RT kernel

Add the following both to committed `yocto/config/local.conf.append` and to the currently active `yocto/build/conf/local.conf`:

```conf
INIT_MANAGER = "systemd"
PREFERRED_PROVIDER_virtual/kernel = "linux-yocto-rt"
```

`INIT_MANAGER = "systemd"` provides the service manager needed for AdaptivePi processes, dependencies, restart policies, logging, and lifecycle control.

`PREFERRED_PROVIDER_virtual/kernel = "linux-yocto-rt"` selects Yocto's PREEMPT_RT kernel provider.

Verify the available RT kernel recipe and provider resolution:

```bash
bitbake-layers show-recipes linux-yocto-rt
bitbake -e virtual/kernel | grep -E '^(PN|PREFERRED_PROVIDER_virtual/kernel)='
```

The resolved values must identify `linux-yocto-rt`.

### What PREEMPT_RT means

PREEMPT_RT improves scheduling determinism and latency behavior compared with standard general-purpose Linux. It does **not** make this a guaranteed hard real-time system. QEMU can prove that the RT kernel boots and software integrates, but it cannot validate Raspberry Pi latency. That requires measurements on the real board under representative load.

Alternatives:

| Option | Suitable when | Tradeoff |
|---|---|---|
| Standard `linux-yocto` | No latency requirement | Less predictable scheduling. |
| PREEMPT_RT, chosen | Latency-sensitive Linux processes | Requires engineering discipline and real measurements. |
| Dedicated RTOS alongside Linux | Hard real-time loops | More system architecture and integration effort. |

The rationale and limitation are recorded in `docs/adr/0005-preempt-rt-linux-kernel.md`.

## 7. Build the AdaptivePi image safely

```bash
bitbake adaptive-pi-image
```

This build can reuse baseline downloads and sstate output, but it still needs to build or assemble the RT kernel, systemd, OpenSSH, gdbserver, and the final filesystem.

Important operating constraints on the current Arch laptop:

- The initial baseline build took about 14 hours.
- Preserve `yocto/downloads/` and `yocto/sstate-cache/`.
- Do not run `bitbake -c cleanall` unless there is a specific, understood reason.
- Keep Yocto parallelism modest because the laptop experienced memory/swap pressure and temporarily stopped responding.
- Do not start another BitBake build while this one runs.
- A small native CMake build using `-j 1` is acceptable, but avoid other heavy compilation.
- Do not edit active Yocto metadata during a long build; commit it first so the resulting image maps to known source state.

## 8. Hello AdaptivePi host application

The first application is intentionally small:

```text
apps/hello-adaptive/
  CMakeLists.txt
  src/main.cpp
  systemd/hello-adaptive.service
```

The top-level CMake project includes it using:

```cmake
add_subdirectory(apps/hello-adaptive)
```

The established configure preset is `debug-app`:

```bash
cmake --preset debug-app
cmake --build build/debug-app --target hello-adaptive -j 1
./build/debug-app/apps/hello-adaptive/hello-adaptive
```

The successful native result is:

```text
Hello from AdaptivePi on Embedded Linux.
```

This CMake configure step also gives VS Code IntelliSense the compile information it needs. The next Yocto step will package this same executable and its systemd unit into `adaptive-pi-image`.

## 9. QEMU development environment

Use the project scripts to start and stop the AdaptivePi QEMU development target.

Start the target:

```bash
scripts/qemu/start-development-image.sh
```

The script:

- prevents a duplicate QEMU instance;
- starts QEMU in the detached tmux session `adaptive-pi-qemu`;
- uses the current AdaptivePi image with `nographic` and `slirp` networking;
- waits until SSH is reachable;
- returns control to the invoking Bash shell only after the guest is ready.

Expected output:

```text
AdaptivePi QEMU is starting up; please wait for SSH readiness...
AdaptivePi QEMU is running and ready for SSH.
Attach with: tmux attach -t adaptive-pi-qemu
SSH with: ssh -p 2222 root@localhost
```

Connect to the running guest:

```bash
ssh -p 2222 root@localhost
```

Attach to the QEMU serial console when needed:

```bash
tmux attach -t adaptive-pi-qemu
```

Stop the target cleanly:

```bash
scripts/qemu/stop-development-image.sh
```

The script requests `systemctl poweroff` through SSH, waits for QEMU to exit, and removes the tmux session.

Expected output:

```text
AdaptivePi QEMU is stopping; please wait...
AdaptivePi QEMU guest and tmux session stopped.
```

If QEMU is already running, the start script reports that instead of launching a duplicate instance. If no instance is running, the stop script safely reports:

```text
AdaptivePi QEMU is not running.
```

## 10. Next milestones

1. Boot and verify the PREEMPT_RT systemd image in QEMU.
2. Add a Yocto recipe for `hello-adaptive` and its systemd unit.
3. Configure QEMU networking and SSH.
4. Create the Yocto SDK cross-build workflow.
5. Deploy to the QEMU guest and debug with host GDB plus target `gdbserver`.
6. Build focused Adaptive-style runtime components: `ara::core`, `ara::log`, `ara::exec`, then `ara::com`.
7. Add Raspberry Pi-specific metadata and validate real hardware, peripherals, and latency.
