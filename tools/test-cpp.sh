#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT
compiler=${CXX:-c++}
"$compiler" -std=c++11 -fsanitize=address,undefined -g test/marquee.cpp -o "$build/marquee"
"$build/marquee"
printf 'PASS: title marquee tests\n'
