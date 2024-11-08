#!/usr/bin/env sh

echo 'conf all'

for bt in "debug" "debugoptimized" "release";
do
    if [ ! -d "build/$bt" ]
    then
        CC=clang-15 CXX=clang++-15 CXX_LD=mold meson\
            setup build/$bt\
            --buildtype $bt
    fi
done
