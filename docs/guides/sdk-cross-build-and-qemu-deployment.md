# AdaptivePi SDK Cross-Build and QEMU Deployment Guide

This guide documents the daily AdaptivePi application-development workflow:

```text
C++ source on Arch Linux
→ Yocto SDK cross-build for AArch64
→ SSH deployment into QEMU
→ execution in the PREEMPT_RT Poky guest
```

This workflow does not rebuild the Yocto image for every application change.

## Prerequisites

The following must already exist:

- AdaptivePi Yocto image built successfully.
- Yocto SDK generated and installed in `yocto/sdk/`.
- QEMU lifecycle scripts available under `scripts/qemu/`.
- A Bash terminal open at the repository root.

The Yocto image-build procedure is documented separately in:

```text
docs/guides/yocto-qemu-preempt-rt-build-guide.md
```

## 1. Activate the Yocto SDK

Open a new Bash terminal at the repository root:

```bash
cd ~/workspace/adaptive-pi
```

Source the SDK environment script:

```bash
source yocto/sdk/environment-setup-cortexa57-poky-linux
```

This must be done in every new terminal that performs a cross-build.

The SDK provides:

- `aarch64-poky-linux-g++`, the target C++ compiler;
- the matching ARM64 target sysroot;
- target headers and libraries;
- Yocto-compatible compiler flags and linker configuration;
- cross-debugging tools.

Verify the target sysroot and compiler:

```bash
echo "$SDKTARGETSYSROOT"
aarch64-poky-linux-g++ --version
```

Expected facts:

- the sysroot path ends with `cortexa57-poky-linux`;
- the compiler identifies as `aarch64-poky-linux-g++`.

## 2. Configure and build the AArch64 application

The `debug-qemu-app` CMake preset contains the Yocto SDK toolchain path and required cross-compilation environment.

Configure and build:

```bash
cmake --preset debug-qemu-app
cmake --build --preset debug-qemu-app
```

The resulting binaries are written below:

```bash
build/debug-qemu-app/
```

The preset disables clang-tidy only for the cross-build because the host-installed analysis tool cannot interpret Yocto ARM64 compiler flags and sysroot headers. Native application and unit-test presets continue to use clang-tidy.

Update the deploy example to:

```bash
scripts/qemu/deploy-binary.sh \
  build/debug-qemu-app/apps/hello-adaptive/hello-adaptive
```

## 3. Cross-build an application

The QEMU preset builds the application into:

```text
build/debug-qemu-app/apps/hello-adaptive/hello-adaptive

Build the target application:

```bash
cmake --build build/debug-qemu-app --target hello-adaptive 
```

Verify that the output is an ARM64 executable:

```bash
file build/sdk-aarch64/apps/hello-adaptive/hello-adaptive
```

Expected result:

```text
ELF 64-bit ... ARM aarch64 ...
```

The host cannot run this executable directly because it is built for the ARM64 Poky target.

## 4. Start the QEMU target

Start the managed QEMU development target:

```bash
scripts/qemu/start-development-image.sh
```

The script:

- starts QEMU in a detached tmux session;
- prevents duplicate QEMU instances;
- waits until guest SSH is ready;
- prints the SSH connection command.

Connect manually when needed:

```bash
ssh -p 2222 root@localhost
```

## 5. Deploy the target binary

Deploy a cross-built binary to the running guest:

```bash
scripts/qemu/deploy-binary.sh \
  build/debug-qemu-app/apps/hello-adaptive/hello-adaptive
```

The deployment script:

- verifies that the managed QEMU tmux session exists;
- verifies that guest SSH is ready;
- creates `/usr/local/bin` in the guest if necessary;
- copies the binary through SCP;
- sets executable permission;
- installs the binary using its local filename.

For this example, the guest destination is:

```text
/usr/local/bin/hello-adaptive
```

## 6. Execute the binary in QEMU

Run the deployed ARM64 application through SSH:

```bash
ssh -p 2222 root@localhost /usr/local/bin/hello-adaptive
```

Expected output:

```text
Hello from AdaptivePi on Embedded Linux.
```

This proves that the application was cross-built with the Yocto SDK, deployed over SSH, and executed on the ARM64 PREEMPT_RT target.

## 7. Stop the QEMU target

Stop QEMU cleanly:

```bash
scripts/qemu/stop-development-image.sh
```

The script sends `systemctl poweroff` to the guest, waits for QEMU to exit, and removes its tmux session.

Use forced shutdown only when the guest is stuck or unreachable:

```bash
scripts/qemu/stop-development-image.sh --force
```

`--force` terminates the tmux/QEMU session only after clean shutdown cannot complete.

## 8. VS Code remote debugging

AdaptivePi supports source-level debugging of an AArch64 process in the QEMU PREEMPT_RT guest directly from VS Code.

The configuration is defined by:

```text
.vscode/launch.json
.vscode/tasks.json
```

The launch configuration uses the Yocto SDK cross-GDB executable and target sysroot. VS Code tasks build, deploy, and prepare the SSH-backed remote debug connection automatically.

### Start a debug session

Start QEMU first:

```bash
scripts/qemu/start-development-image.sh
```

In VS Code:

1. Open `apps/hello-adaptive/src/main.cpp`.
2. Set a source breakpoint.
3. Open **Run and Debug**.
4. Select `Debug: QEMU Hello Adaptive`.
5. Press `F5`.

VS Code automatically performs:

```text
build ARM64 binary
→ deploy binary into QEMU
→ enable guest gdbserver
→ create host SSH tunnel
→ attach Yocto cross-GDB
```

The debugger first pauses in the Linux dynamic loader. This is normal: it allows VS Code to register source breakpoints before the application begins.

Press Continue to reach the application breakpoint.

### Stop a debug session

Stopping VS Code debugging automatically runs:

```bash
scripts/qemu/disable-debug-session.sh
```

This removes the host SSH tunnel and stops guest `gdbserver`.

Stop QEMU when development is complete:

```bash
scripts/qemu/stop-development-image.sh
```

The debug-session scripts are also available for manual use:

```bash
scripts/qemu/enable-debug-session.sh \
  build/debug-qemu-app/apps/hello-adaptive/hello-adaptive

scripts/qemu/disable-debug-session.sh
```

The enable script prevents duplicate debug sessions and reports if host port `2345` is already in use.

## Daily development loop

For normal application changes:

1. Open a new Bash terminal and source the SDK.
2. Build the affected AArch64 application.
3. Start QEMU.
4. Deploy the updated binary.
5. Run or debug it in the guest.
6. Stop QEMU when finished.

Yocto image rebuilds are reserved for changes to image packages, kernel configuration, system services, or reproducible release-image contents.