#!/usr/bin/env bash
set -euo pipefail

tmux_session="adaptive-pi-qemu"
ssh_port="2222"
guest_binary_directory="/usr/local/bin"

usage() {
    echo "Usage: $0 <local-binary>" >&2
}

if [[ $# -ne 1 ]]; then
    usage
    exit 2
fi

local_binary="$1"
binary_name="$(basename "${local_binary}")"
guest_destination="${guest_binary_directory}/${binary_name}"

ssh_options=(
    -o BatchMode=yes
    -o ConnectTimeout=5
    -o StrictHostKeyChecking=accept-new
    -p "${ssh_port}"
)

if [[ ! -f "${local_binary}" ]]; then
    echo "Local binary was not found: ${local_binary}" >&2
    exit 1
fi

if ! tmux has-session -t "${tmux_session}" 2>/dev/null; then
    echo "AdaptivePi QEMU is not running." >&2
    echo "Start it with: scripts/qemu/start-development-image.sh" >&2
    exit 1
fi

if ! ssh "${ssh_options[@]}" root@localhost true >/dev/null 2>&1; then
    echo "AdaptivePi QEMU is running, but SSH is not ready." >&2
    exit 1
fi

ssh "${ssh_options[@]}" root@localhost \
    "mkdir -p '${guest_binary_directory}'"

scp -P "${ssh_port}" \
    "${local_binary}" \
    "root@localhost:${guest_destination}"

ssh "${ssh_options[@]}" root@localhost \
    "chmod 0755 '${guest_destination}'"

echo "AdaptivePi binary deployed successfully: ${guest_destination}"