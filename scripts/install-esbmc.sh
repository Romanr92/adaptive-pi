#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
install_dir="${1:-${project_root}/build/tools/esbmc}"
if (( $# > 1 )); then
    echo "Usage: $0 [installation-directory]" >&2
    exit 2
fi

if [[ -x "${install_dir}/bin/esbmc" ]]; then
    "${install_dir}/bin/esbmc" --version
    exit 0
fi

version="8.5"
case "$(uname -s):$(uname -m)" in
    Linux:x86_64)
        archive="esbmc-linux.zip"
        checksum="d8da304dd0dfce6c9f488379f03a72e768bce8598c22478c980e1704247352f1"
        ;;
    Linux:aarch64|Linux:arm64)
        archive="esbmc-linux-armv8.zip"
        checksum="5470aac77f2057f60b232c95dfe0b9b71fef4e736246a53aeb19aaa549dd37f7"
        ;;
    *)
        echo "Automatic ESBMC installation supports Linux x86_64 and ARM64 hosts." >&2
        echo "Install ESBMC manually and set ESBMC_EXECUTABLE in CMake." >&2
        exit 1
        ;;
esac

for tool in curl unzip sha256sum flock realpath; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
        echo "ESBMC installation requires ${tool}." >&2
        exit 1
    fi
done

mkdir -p -- "$(dirname -- "${install_dir}")"
install_dir="$(realpath -m -- "${install_dir}")"
# Serialize concurrent CMake configurations using the same destination.
exec 9>"${install_dir}.lock"
flock 9
if [[ -x "${install_dir}/bin/esbmc" ]]; then
    "${install_dir}/bin/esbmc" --version
    exit 0
fi
if [[ -e "${install_dir}" || -L "${install_dir}" ]]; then
    echo "Refusing to overwrite incomplete ESBMC installation: ${install_dir}" >&2
    echo "Choose a new destination or remove the incomplete installation first." >&2
    exit 1
fi

staging_dir="$(mktemp -d "${install_dir}.tmp.XXXXXX")"
trap 'rm -rf -- "${staging_dir}"' EXIT
echo "Downloading ESBMC ${version} (${archive})"
curl --fail --location --show-error --retry 3 --connect-timeout 20 --max-time 600 \
    "https://github.com/esbmc/esbmc/releases/download/v${version}/${archive}" \
    --output "${staging_dir}/esbmc.zip"
if ! printf '%s  %s\n' "${checksum}" "${staging_dir}/esbmc.zip" | sha256sum --check --status; then
    echo "ESBMC archive checksum mismatch; installation aborted." >&2
    exit 1
fi
unzip -q "${staging_dir}/esbmc.zip" -d "${staging_dir}/unpacked"

# Keep the complete distribution, including its bundled headers and licences.
mapfile -t executables < <(find "${staging_dir}/unpacked" -type f -path '*/bin/esbmc')
if (( ${#executables[@]} != 1 )); then
    echo "Expected exactly one bin/esbmc in the release archive." >&2
    exit 1
fi
chmod +x "${executables[0]}"
"${executables[0]}" --version
distribution_dir="$(dirname -- "$(dirname -- "${executables[0]}")")"
mv -- "${distribution_dir}" "${install_dir}"
echo "Installed ESBMC in ${install_dir}"
