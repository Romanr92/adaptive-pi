#!/usr/bin/env bash
set -euo pipefail

tmux_session="adaptive-pi-qemu"
ssh_port="2222"

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
yocto_dir="${project_root}/yocto"
build_dir="${yocto_dir}/build"
qemuboot_conf="${build_dir}/tmp/deploy/images/qemuarm64/adaptive-pi-image-qemuarm64.rootfs.qemuboot.conf"

ssh_options=(
    -o BatchMode=yes
    -o ConnectTimeout=2
    -o StrictHostKeyChecking=accept-new
    -p "${ssh_port}"
)

platform_runner="${yocto_dir}/platform/run-platform.py"

if [[ ! -f "${platform_runner}" && ! -f "${qemuboot_conf}" ]]; then
    echo "AdaptivePi QEMU boot configuration was not found:" >&2
    echo "  ${qemuboot_conf}" >&2
    exit 1
fi

if tmux has-session -t "${tmux_session}" 2>/dev/null; then
    echo "AdaptivePi QEMU is already running in tmux session '${tmux_session}'."
    echo "Attach with: tmux attach -t ${tmux_session}"
    echo "SSH with: ssh -p ${ssh_port} root@localhost"
    exit 1
fi

tmux_command="bash -lc 'source \"${yocto_dir}/poky/oe-init-build-env\" \"${build_dir}\" >/dev/null && exec runqemu \"${qemuboot_conf}\" nographic slirp'"

if [[ -f "${platform_runner}" ]]; then
    printf -v tmux_command 'python3 %q' "${platform_runner}"
fi

echo "AdaptivePi QEMU is starting up; please wait for SSH readiness..."

tmux new-session -d -s "${tmux_session}" "${tmux_command}"

for ((attempt = 1; attempt <= 60; ++attempt)); do
    if ssh "${ssh_options[@]}" root@localhost "true" >/dev/null 2>&1; then
        echo "AdaptivePi QEMU is running and ready for SSH."
        echo "Attach with: tmux attach -t ${tmux_session}"
        echo "SSH with: ssh -p ${ssh_port} root@localhost"
        exit 0
    fi

    if ! tmux has-session -t "${tmux_session}" 2>/dev/null; then
        echo "AdaptivePi QEMU stopped before SSH became ready." >&2
        exit 1
    fi

    sleep 1
done

echo "AdaptivePi QEMU started, but SSH was not ready within 60 seconds." >&2
echo "Inspect with: tmux attach -t ${tmux_session}" >&2
exit 1
