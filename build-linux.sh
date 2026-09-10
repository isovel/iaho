#!/usr/bin/env bash
# Cross-compile the Windows DLL from Linux using clang-cl + xwin.
# Prereqs: cmake >= 3.22, ninja, LLVM >= 17 (clang-cl/lld-link/llvm-lib),
#          rust >= 1.73 with the x86_64-pc-windows-msvc target,
#          xwin splat output (see README).
set -euo pipefail

CONFIG="${1:-Game__Shipping__Win64}"
BUILD_DIR="${BUILD_DIR:-build_xwin}"
export XWIN_DIR="${XWIN_DIR:-$HOME/.xwin}"
export PATH="$HOME/.cargo/bin:$HOME/.local/bin:${LLVM_ROOT:-/usr/lib/llvm-19}/bin:$PATH"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cmake -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE="$CONFIG" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/deps/ue4ss/cmake/toolchains/xwin-clang-cl-toolchain.cmake"

# Only our target: UE4SS's proxy_generator step runs a cross-built .exe on the
# host, which Linux cannot exec. The mod DLL does not need it.
cmake --build "$BUILD_DIR" --target IAlreadyHaveOne

# Deploy into the game's Mods folder unless NO_INSTALL is set.
INSTALL="$ROOT/install.sh"

if [[ -n "${NO_INSTALL:-}" ]]; then
    :
elif [[ -x "$INSTALL" ]]; then
    "$INSTALL"
else
    echo "build: no install.sh, leaving the DLL in dlls/ (see README)" >&2
fi
