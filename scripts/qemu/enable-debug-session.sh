#!/usr/bin/env bash
set -euo pipefail

tmux_session="adaptive-pi-qemu"
ssh_port="2222"
gdb_port="2345"

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
deploy_script="${project_root}/scripts/qemu/deploy-binary.sh"
state_directory="${project_root}/build/qemu-debug"
tunnel_pid_file="${state_directory}/ssh-tunnel.pid"

guest_pid_file="/tmp/adaptive-pi-gdbserver.pid"
guest_log_file="/tmp/adaptive-pi-gdbserver.log"

usage() {
    echo "Usage: $0 <local-arm64-binary>" >&2
}

if [[ $# -ne 1 ]]; then
    usage
    exit 2
fi

local_binary="$1"
binary_name="$(basename "${local_binary}")"
guest_binary="/usr/local/bin/${binary_name}"

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

if [[ -f "${tunnel_pid_file}" ]]; then
    read -r tunnel_pid < "${tunnel_pid_file}"

    if kill -0 "${tunnel_pid}" 2>/dev/null; then
        echo "AdaptivePi debug session is already running." >&2
        exit 1
    fi

    rm -f "${tunnel_pid_file}"
fi

if ss -ltn "sport = :${gdb_port}" | grep -q "^LISTEN"; then
    echo "Host port ${gdb_port} is already in use." >&2
    echo "Stop the existing debug tunnel before starting another session." >&2
    exit 1
fi

if ssh "${ssh_options[@]}" root@localhost \
    "test -s ${guest_pid_file} && kill -0 \$(cat ${guest_pid_file})" \
    >/dev/null 2>&1; then
    echo "AdaptivePi guest gdbserver is already running." >&2
    exit 1
fi

"${deploy_script}" "${local_binary}"

echo "Starting AdaptivePi guest gdbserver..."

ssh "${ssh_options[@]}" root@localhost \
    "nohup gdbserver 127.0.0.1:${gdb_port} '${guest_binary}' \
    > '${guest_log_file}' 2>&1 < /dev/null & \
    echo \$! > '${guest_pid_file}'"

sleep 1

if ! ssh "${ssh_options[@]}" root@localhost \
    "test -s ${guest_pid_file} && kill -0 \$(cat ${guest_pid_file})" \
    >/dev/null 2>&1; then
    echo "AdaptivePi guest gdbserver did not start." >&2
    ssh "${ssh_options[@]}" root@localhost \
        "cat '${guest_log_file}' 2>/dev/null || true" >&2
    exit 1
fi

mkdir -p "${state_directory}"

nohup ssh "${ssh_options[@]}" \
    -N \
    -o ExitOnForwardFailure=yes \
    -L "${gdb_port}:127.0.0.1:${gdb_port}" \
    root@localhost \
    >/dev/null 2>&1 &

tunnel_pid=$!

sleep 1

if ! kill -0 "${tunnel_pid}" 2>/dev/null; then
    ssh "${ssh_options[@]}" root@localhost \
        "kill \$(cat '${guest_pid_file}') 2>/dev/null || true; \
        rm -f '${guest_pid_file}'" || true

    echo "AdaptivePi SSH debug tunnel did not start." >&2
    exit 1
fi

printf '%s\n' "${tunnel_pid}" > "${tunnel_pid_file}"

echo "AdaptivePi debug session is ready."
echo "VS Code: start 'Debug: QEMU Hello Adaptive'."