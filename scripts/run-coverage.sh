#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${1:-${project_root}/build/coverage-check}"
# Optional second argument selects the measured exception configuration.
exception_mode="${2:-ON}"
case "${exception_mode}" in
    ON|OFF) ;;
    *) echo "Usage: $0 [build-directory] [ON|OFF]" >&2; exit 2 ;;
esac
site_dir="${build_dir}/site"
coverage_dir="${site_dir}/coverage"

mkdir -p "${build_dir}"

if [[ -d "${project_root}/site" && "${project_root}/site" != "${site_dir}" ]]; then
    rm -rf "${project_root}/site"
fi

# A compiler change in a reused cache can silently reset the coverage flags.
cmake --fresh -S "${project_root}" -B "${build_dir}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTING=ON \
    -DADAPTIVE_PI_ENABLE_EXCEPTIONS="${exception_mode}" \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_CXX_FLAGS='--coverage -O0 -g' \
    -DCMAKE_C_FLAGS='--coverage -O0 -g' \
    -DCMAKE_EXE_LINKER_FLAGS='--coverage' \
    -DCMAKE_SHARED_LINKER_FLAGS='--coverage' \
    -DADAPTIVE_PI_ENABLE_CLANG_TIDY=OFF \
    -DADAPTIVE_PI_WARNINGS_AS_ERRORS=ON

# Discard counters from earlier runs before rebuilding and executing the tests.
find "${build_dir}" -type f -name '*.gcda' -delete

cmake --build "${build_dir}" --parallel
ctest --test-dir "${build_dir}" --output-on-failure

LLVM_COV_BIN="$(command -v llvm-cov || command -v llvm-cov-18 || command -v llvm-cov-17 || true)"
if [[ -z "${LLVM_COV_BIN}" ]]; then
    echo "llvm-cov is required for the coverage build but was not found in PATH." >&2
    exit 1
fi

mkdir -p "${coverage_dir}"

gcovr -r "${project_root}" "${build_dir}" \
    --root "${project_root}" \
    --gcov-executable "${LLVM_COV_BIN} gcov" \
    --filter '^(apps|platform)/.*\.(c|cpp|cxx|h|hpp|hxx)$' \
    --exclude '^build/' \
    --exclude '^yocto/' \
    --exclude '^(apps|platform)/(.*/)?tests?/' \
    --exclude '^(apps|platform)/(.*/)?(test_[^/]*|[^/]*_(test|tests|spec))\.(c|cpp|cxx|h|hpp|hxx)$' \
    --gcov-ignore-errors=source_not_found \
    --gcov-ignore-errors=output_error \
    --gcov-ignore-errors=no_working_dir_found \
    --html-details "${coverage_dir}/index.html" \
    --json-summary "${coverage_dir}/summary.json" \
    --print-summary

cat > "${site_dir}/index.html" <<HTML
<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <title>AdaptivePi coverage</title>
  </head>
  <body>
    <a href="coverage/">View unit-test coverage</a>
  </body>
</html>
HTML

echo "Coverage report ready: ${coverage_dir}/index.html"
