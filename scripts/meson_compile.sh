#!/usr/bin/env sh

# Usage: ./scripts/ninja.sh [debug/debugoptimized/release] [wheels_bench/wheels_test]

cwd=$(pwd)
script_dir=$( dirname -- "$( readlink -f -- "$0"; )"; )
cd $script_dir/..

./scripts/meson_configure.sh

meson compile $2 -C build/$1

cd $cwd
