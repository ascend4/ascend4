#!/bin/bash
set -e

SRC=/mnt/ascend
WORK=/work/ascend

if [ ! -d "$SRC" ]; then
    echo "Expected source tree at $SRC"
    echo "Run with: -v \$HOME/ascend:$SRC:ro"
    exit 1
fi

mkdir -p /work

if [ ! -d "$WORK/.git" ]; then
    echo "Initialising writable working copy from read-only host mount..."
    rsync -a --delete \
        --exclude '.git/' \
        --exclude '.sconsign.dblite' \
        --exclude 'build/' \
        "$SRC/" "$WORK/"
else
    echo "Refreshing writable working copy from read-only host mount..."
    rsync -a --delete \
        --exclude '.git/' \
        --exclude '.sconsign.dblite' \
        --exclude 'build/' \
        "$SRC/" "$WORK/"
fi

cd "$WORK"
exec "$@"
