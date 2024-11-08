#!/usr/bin/env sh

# Usage: ./scripts/ninja.sh [debug/debugoptimized/release] [wheels_bench/wheels_test]

./meson_configure_all.sh

cd build/$1

meson compile $2

cd ../../
