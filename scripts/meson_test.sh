#!/usr/bin/env sh

# Usage: meson_test.sh [debug/debugoptimized/release]

cwd=$(pwd)
script_dir=$( dirname -- "$( readlink -f -- "$0"; )"; )
cd $script_dir/..

./scripts/meson_configure.sh

meson test wheels -C build/$1

cd $cwd
