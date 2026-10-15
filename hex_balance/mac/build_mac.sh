#!/bin/bash
# Builds the hex prototype of Commander Wars on a Mac. Run it from the folder of the unpacked source:
#
#     bash hex_balance/mac/build_mac.sh
#
# It needs the command line tools of Xcode (xcode-select --install) and Homebrew (https://brew.sh).
# The first build takes a while. Afterwards join a match with hex_balance/mac/join_mac.sh.
set -e
cd "$(dirname "$0")/../.."

if ! command -v brew >/dev/null 2>&1; then
    echo "Homebrew is missing. Install it from https://brew.sh and start this script again."
    exit 1
fi
if ! xcode-select -p >/dev/null 2>&1; then
    echo "The command line tools of Xcode are missing. Run: xcode-select --install"
    exit 1
fi

echo "== installing the build tools and libraries (skipped if they are there)"
for package in cmake ninja qt openssl@3; do
    brew list "$package" >/dev/null 2>&1 || brew install "$package"
done

QT_DIR="$(brew --prefix qt)"
OPENSSL_DIR="$(brew --prefix openssl@3)"
echo "== Qt: $QT_DIR"
echo "== OpenSSL: $OPENSSL_DIR"

# The resources stay a folder next to the source, like on the computer of the host: the game checks
# that the scripts of both computers are the same when you join.
cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$QT_DIR" \
    -DOPENSSL_ROOT_DIR="$OPENSSL_DIR" \
    -DOPENSSL_USE_STATIC_LIBS=OFF \
    -DDEPLOY_RESOURCES_AS_FOLDER=ON \
    -DUPDATESUPPORT=OFF 2>&1 | tee build_mac_configure.log

echo "== building, the output is also written to build_mac.log"
set +e
cmake --build build --target Commander_Wars -j "$(sysctl -n hw.ncpu)" 2>&1 | tee build_mac.log
set -e

BINARY="build/Commander_Wars.app/Contents/MacOS/Commander_Wars"
if [ ! -x "$BINARY" ]; then
    BINARY="build/Commander_Wars"
fi
if [ -x "$BINARY" ]; then
    echo
    echo "== done: $BINARY"
    echo "   (errors of macdeployqt or codesign at the very end don't matter for playing from this folder)"
    echo "   join a match with: bash hex_balance/mac/join_mac.sh <address of the host>"
else
    echo
    echo "== the build failed. Send build_mac_configure.log and build_mac.log to the host."
    exit 1
fi
