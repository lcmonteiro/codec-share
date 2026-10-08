#!/usr/bin/env bash
# =======================================================================================
# builds codec-share.wasm, a WASI reactor exposing the C interface of codec_share.cpp
#
#   WASI_SDK_PATH=/opt/wasi-sdk ./wasm/build.sh [output]
#
# wasi-sdk: https://github.com/WebAssembly/wasi-sdk/releases
# =======================================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SDK="${WASI_SDK_PATH:-/opt/wasi-sdk}"
OUT="${1:-$ROOT/build/codec-share.wasm}"

[ -x "$SDK/bin/clang++" ] || { echo "wasi-sdk not found, set WASI_SDK_PATH" >&2; exit 1; }
mkdir -p "$(dirname "$OUT")"

"$SDK/bin/clang++" \
    --target=wasm32-wasip1 --sysroot="$SDK/share/wasi-sysroot" \
    -std=c++17 -O2 -fno-exceptions -Wall -Wextra \
    -mexec-model=reactor -Wl,--strip-all \
    -I "$ROOT/include" \
    -o "$OUT" "$ROOT/wasm/codec_share.cpp"

echo "$OUT"
