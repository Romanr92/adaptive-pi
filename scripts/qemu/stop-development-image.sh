#!/usr/bin/env bash
set -euo pipefail

tmux_session="adaptive-pi-qemu"
ssh_port="2222"
force_shutdown=false

usage() {
    echo "Usage: $0 [--force]" >&2
}

if [[ $# -eq 1 && "$1" == "--force" ]]; then
    force_shutdown=true
elif [[ $# -ne 0 ]]; then
    usage
    exit 2
fi

ssh_options=(
    -o BatchMode=yes
    -o ConnectTimeout=5
    -o StrictHostKeyChecking=accept-new
    -p "${ssh_port}"
)

if ! tmux has-session -t "${tmux_session}" 2>/dev/null; then
    echo "AdaptivePi QEMU is not running."
    exit 0
fi

echo "AdaptivePi QEMU is stopping; please wait..."

if ! ssh "${ssh_options[@]}" root@localhost "systemctl poweroff"; then
    if [[ "${force_shutdown}" == true ]]; then
        echo "SSH is unavailable; forcing QEMU shutdown."
        tmux kill-session -t "${tmux_session}"
        echo "AdaptivePi QEMU tmux session was force-stopped."
        exit 0
    fi

    echo "AdaptivePi QEMU is running, but SSH is unavailable." >&2
    echo "A clean shutdown cannot be requested automatically." >&2
    echo "Use '$0 --force' only when a forced stop is necessary." >&2
    exit 1
fi

for ((attempt = 1; attempt <= 30; ++attempt)); do
    if ! tmux has-session -t "${tmux_session}" 2>/dev/null; then
        echo "AdaptivePi QEMU guest and tmux session stopped."
        exit 0
    fi

    sleep 1
done

if [[ "${force_shutdown}" == true ]]; then
    echo "Guest shutdown timed out; forcing QEMU shutdown."
    tmux kill-session -t "${tmux_session}"
    echo "AdaptivePi QEMU tmux session was force-stopped."
    exit 0
fi

echo "Timed out while waiting for the AdaptivePi QEMU guest to stop." >&2
echo "The tmux session was left running to avoid a forced shutdown." >&2
echo "Use '$0 --force' only when a forced stop is necessary." >&2
exit 1