#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 <path-to-sdk-installer.sh>" >&2
}

if [[ $# -ne 1 ]]; then
    usage
    exit 2
fi

sdk_installer="$1"

if [[ ! -f "${sdk_installer}" ]]; then
    echo "SDK installer was not found: ${sdk_installer}" >&2
    exit 1
fi

if [[ "${sdk_installer}" != *.sh ]]; then
    echo "SDK installer must be a .sh file: ${sdk_installer}" >&2
    exit 1
fi

sdk_directory="$(cd -- "$(dirname -- "${sdk_installer}")" && pwd)"
sdk_filename="$(basename -- "${sdk_installer}")"
checksum_filename="${sdk_filename}.sha256"

(
    cd -- "${sdk_directory}"
    sha256sum -- "${sdk_filename}" > "${checksum_filename}"
)

echo "SDK checksum created: ${sdk_directory}/${checksum_filename}"