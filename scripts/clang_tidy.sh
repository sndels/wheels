#!/usr/bin/env sh

cwd=$(pwd)
script_dir=$( dirname -- "$( readlink -f -- "$0"; )"; )
cd $script_dir/..

./scripts/meson_configure.sh

run-clang-tidy-15 -p='build/debug' 'wheels/tests/.*.cpp'

cd $cwd
