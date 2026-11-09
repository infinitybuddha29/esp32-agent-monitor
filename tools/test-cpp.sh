#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT
compiler=${CXX:-c++}
"$compiler" -std=c++11 -fsanitize=address,undefined -g test/marquee.cpp -o "$build/marquee"
"$build/marquee"
"$compiler" -std=c++11 -fsanitize=address,undefined -g -Ipixel-office-demo/test \
    pixel-office-demo/test/choreography.cpp firmware/PixelOfficeDemo/src/office_state.cpp \
    -o "$build/choreography"
"$build/choreography"
