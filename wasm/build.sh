#!/usr/bin/env bash
# =======================================================================================
# builds codec-share.wasm, a WASI reactor exposing the C interface of codec_share.cpp
#
#   WASI_SDK_PATH=/opt/wasi-sdk ./wasm/build.sh [output]
#
# the default output, wasm/codec-share.wasm, is committed: rebuild it with every change
# of the sources (CI checks it). The build is reproducible with the pinned wasi-sdk.
#
# wasi-sdk: https://github.com/WebAssembly/wasi-sdk/releases
# =======================================================================================
set -euo pipefail

WASI_SDK_VERSION=25.0

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SDK="${WASI_SDK_PATH:-/opt/wasi-sdk}"
OUT="${1:-$ROOT/wasm/codec-share.wasm}"

[ -x "$SDK/bin/clang++" ] || { echo "wasi-sdk not found, set WASI_SDK_PATH" >&2; exit 1; }
if [ "$(head -n1 "$SDK/VERSION" 2>/dev/null)" != "$WASI_SDK_VERSION" ]; then
    echo "warning: wasi-sdk $WASI_SDK_VERSION expected, the output will differ from CI" >&2
fi
mkdir -p "$(dirname "$OUT")"

"$SDK/bin/clang++" \
    --target=wasm32-wasip1 --sysroot="$SDK/share/wasi-sysroot" \
    -std=c++17 -O2 -fno-exceptions -Wall -Wextra -Werror \
    -mexec-model=reactor -Wl,--strip-all \
    -I "$ROOT/include" \
    -o "$OUT" "$ROOT/wasm/codec_share.cpp"

echo "$OUT"
