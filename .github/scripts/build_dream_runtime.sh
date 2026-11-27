#!/usr/bin/env bash
set -euo pipefail

PLATFORM="${1:-}"
ARCH="${2:-}"

if [[ -z "$PLATFORM" || -z "$ARCH" ]]; then
    echo "Usage: $0 <platform> <arch>" >&2
    exit 2
fi

case "$PLATFORM:$ARCH" in
    android:arm64|linux:x86_64) ;;
    *)
        echo "Unsupported Dream Runtime target: $PLATFORM/$ARCH" >&2
        exit 2
        ;;
esac

ROOT_DIR="$(git rev-parse --show-toplevel)"
EXTENSION_DIR="$ROOT_DIR/native/dream_runtime"
GODOT_CPP_DIR="$EXTENSION_DIR/godot-cpp"
GODOT_CPP_COMMIT="507ed9d840c01a3c5b2a39af8bb4000bfac30bf5"

if [[ ! -f "$GODOT_CPP_DIR/SConstruct" ]] || \
   [[ "$(git -C "$GODOT_CPP_DIR" rev-parse HEAD 2>/dev/null || true)" != "$GODOT_CPP_COMMIT" ]]; then
    rm -rf "$GODOT_CPP_DIR"
    git clone --quiet --depth 1 --branch 10.0.0-stable \
        https://github.com/godotengine/godot-cpp.git "$GODOT_CPP_DIR"
fi

ACTUAL_COMMIT="$(git -C "$GODOT_CPP_DIR" rev-parse HEAD)"
if [[ "$ACTUAL_COMMIT" != "$GODOT_CPP_COMMIT" ]]; then
    echo "godot-cpp pin mismatch: expected $GODOT_CPP_COMMIT, got $ACTUAL_COMMIT" >&2
    exit 1
fi

if ! command -v scons >/dev/null 2>&1; then
    echo "SCons is required to build Dream Runtime." >&2
    exit 1
fi

echo "Building Dream Runtime for $PLATFORM/$ARCH with godot-cpp $ACTUAL_COMMIT"
scons -C "$EXTENSION_DIR" \
    platform="$PLATFORM" \
    arch="$ARCH" \
    target=template_debug \
    precision=single \
    -j"$(nproc)"

case "$PLATFORM:$ARCH" in
    android:arm64)
        OUTPUT="$EXTENSION_DIR/bin/android/libdream_runtime.android.template_debug.arm64.so"
        ;;
    linux:x86_64)
        OUTPUT="$EXTENSION_DIR/bin/linux/libdream_runtime.linux.template_debug.x86_64.so"
        ;;
esac

test -s "$OUTPUT"
file "$OUTPUT"
echo "Dream Runtime library ready: $OUTPUT"
