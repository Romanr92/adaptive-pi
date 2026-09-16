#!/usr/bin/env bash
set -euo pipefail

tmux_session="adaptive-pi-qemu"

ssh_options=(
    -o BatchMode=yes
    -o ConnectTimeout=5
    -p 2222
)

if ! tmux has-session -t "${tmux_session}" 2>/dev/null; then
    echo "AdaptivePi QEMU is not running."
    exit 0
fi

echo "AdaptivePi QEMU is stopping; please wait..."

ssh "${ssh_options[@]}" root@localhost "systemctl poweroff" || true

for ((attempt = 1; attempt <= 30; ++attempt)); do
    if ! ssh "${ssh_options[@]}" root@localhost "true" >/dev/null 2>&1; then
        for ((grace = 1; grace <= 5; ++grace)); do
            if ! tmux has-session -t "${tmux_session}" 2>/dev/null; then
                echo "AdaptivePi QEMU guest and tmux session stopped."
                exit 0
            fi

            sleep 1
        done

        tmux kill-session -t "${tmux_session}"
        echo "AdaptivePi guest stopped; remaining tmux session removed."
        exit 0
    fi

    sleep 1
done

echo "Timed out while waiting for the AdaptivePi QEMU guest to stop." >&2
exit 1