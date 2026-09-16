#!/usr/bin/env bash
set -euo pipefail

tmux_session="adaptive-pi-qemu"
ssh_port="2222"

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
state_directory="${project_root}/build/qemu-debug"
tunnel_pid_file="${state_directory}/ssh-tunnel.pid"

guest_pid_file="/tmp/adaptive-pi-gdbserver.pid"

ssh_options=(
    -o BatchMode=yes
    -o ConnectTimeout=5
    -o StrictHostKeyChecking=accept-new
    -p "${ssh_port}"
)

stopped_something=false

if [[ -f "${tunnel_pid_file}" ]]; then
    read -r tunnel_pid < "${tunnel_pid_file}"

    if kill -0 "${tunnel_pid}" 2>/dev/null; then
        kill "${tunnel_pid}"
        echo "AdaptivePi SSH debug tunnel stopped."
        stopped_something=true
    fi

    rm -f "${tunnel_pid_file}"
fi

if tmux has-session -t "${tmux_session}" 2>/dev/null &&
    ssh "${ssh_options[@]}" root@localhost true >/dev/null 2>&1; then
    if ssh "${ssh_options[@]}" root@localhost \
        "test -s ${guest_pid_file} && kill -0 \$(cat ${guest_pid_file})" \
        >/dev/null 2>&1; then
        ssh "${ssh_options[@]}" root@localhost \
            "kill \$(cat '${guest_pid_file}') 2>/dev/null || true; \
            rm -f '${guest_pid_file}'"

        echo "AdaptivePi guest gdbserver stopped."
        stopped_something=true
    else
        ssh "${ssh_options[@]}" root@localhost \
            "rm -f '${guest_pid_file}'" >/dev/null 2>&1 || true
    fi
fi

if [[ "${stopped_something}" == false ]]; then
    echo "AdaptivePi debug session is not running."
fi