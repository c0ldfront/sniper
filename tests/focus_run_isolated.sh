#!/bin/bash
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
run="$root/research/sniper87/runtime-focus"
exec bwrap --unshare-all --die-with-parent --clearenv --setenv PATH /usr/bin --setenv HOME /work/home --setenv USER coldfront --ro-bind /usr /usr --symlink usr/bin /bin --symlink usr/lib /lib --symlink usr/lib /lib64 --proc /proc --dev /dev --tmpfs /tmp --ro-bind /etc /etc --bind "$run" /work --chdir /work /bin/bash /work/run-focus.sh
