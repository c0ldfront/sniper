#!/usr/bin/env bash
set -euo pipefail
root=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
make -C "$root/linux" -f native-linux.mk all

if [[ -n "${DISPLAY:-}" && ! -r "${XAUTHORITY:-}" && -n "${XDG_RUNTIME_DIR:-}" ]]; then
    for auth in "$XDG_RUNTIME_DIR"/xauth_*; do
        if [[ -r "$auth" ]]; then
            export XAUTHORITY="$auth"
            break
        fi
    done
fi
cd "$root/platform/build"
exec "$root/linux/build/native/sniper87-native" "$@"
