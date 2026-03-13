#!/bin/bash
set -e

cd desktop
cmake -B build
cmake --build build --config Release
cd ..

mkdir -p release
find desktop/build/release -maxdepth 1 -type f \( -name "*.exe" -o -name "*.pdb" \) -exec cp {} release/ \;

echo "Finished building pesto-gb, the executable is located in ./release"