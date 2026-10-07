#!/bin/bash
set -e

git submodule update --init --recursive

### --- Version -------------------------------------------------------------
# SEMVER comes in from the CI, whose version job resolves the tag -- and, on
# the pushbuild.txt path, creates it -- before any build starts. The tag is
# pushed by then but this checkout need not have it, so asking git first would
# come up empty and the build would ship as "Local".
#
# A build outside that (a local one, a manual docker compose up) has nothing
# passed in and falls back to whatever tag HEAD carries. An untagged HEAD is
# not an error: CMake stamps it "Local" and the line below says so.
if [ -n "${SEMVER:-}" ]; then
  export SEMVER
  echo "=== Version: $SEMVER (from the environment) ==="
elif SEMVER="$(git describe --tags --exact-match 2>/dev/null)"; then
  export SEMVER
  echo "=== Version: $SEMVER (tag on HEAD) ==="
else
  # Exported-but-empty is not the same as unset to CMake, and would stamp the
  # build with an empty version instead of falling back.
  unset SEMVER
  echo '=== Version: no tag on HEAD, building as "Local" ==='
fi

export GITHUB_SHA_SHORT="$(git rev-parse --short HEAD)"

### --- Download Source2Toolkit-SDK + Metamod-Source ----------------------
SDK_DIR="/tmp/sdk"
SOURCE2TOOLKITSDK_DIR="$SDK_DIR/source2toolkit-sdk"
MMSOURCE_DIR="$SDK_DIR/metamod-source"
CSGO_PROTO_DIR="$SDK_DIR/Protobufs"

echo "=== Preparing temporary SDK directory ==="
rm -rf "$SDK_DIR"
mkdir -p "$SDK_DIR"

echo "=== Downloading Source2Toolkit-SDK ==="
git clone --recursive https://github.com/Source2Toolkit/source2toolkit-sdk.git "$SOURCE2TOOLKITSDK_DIR"
# s2sdk is the SDK's vendor/s2sdk submodule, at the commit the SDK pins.
S2SDK_DIR="$SOURCE2TOOLKITSDK_DIR/vendor/s2sdk"

# S2SDK_REF moves it to another commit when the pinned one does not build;
# empty = the pin.
S2SDK_REF="${S2SDK_REF-}"
if [ -n "$S2SDK_REF" ]; then
  echo "=== Moving s2sdk to $S2SDK_REF ==="
  git -C "$S2SDK_DIR" fetch -q origin "$S2SDK_REF"
  git -C "$S2SDK_DIR" checkout -q FETCH_HEAD
  git -C "$S2SDK_DIR" submodule update -q --init --recursive
fi
echo "s2sdk: $(git -C "$S2SDK_DIR" log -1 --format='%h %s')"

echo "=== Downloading Metamod-Source ==="
git clone --recursive --branch master --single-branch https://github.com/alliedmodders/metamod-source.git "$MMSOURCE_DIR"

# Plugins are built against the SDK's vendor/khook and refuse to load on a core
# built with any other commit, so a drift here must fail the release.
CORE_KHOOK="$(git -C "$MMSOURCE_DIR/third_party/khook" rev-parse HEAD)"
SDK_KHOOK="$(git -C "$SOURCE2TOOLKITSDK_DIR/vendor/khook" rev-parse HEAD)"
echo "KHook: metamod-source $CORE_KHOOK, source2toolkit-sdk $SDK_KHOOK"
if [ "$CORE_KHOOK" != "$SDK_KHOOK" ]; then
  echo "KHook pin drift between metamod-source and source2toolkit-sdk -- bump one to match the other" >&2
  exit 1
fi

echo "=== Downloading Protobufs ==="
git clone --recursive https://github.com/SteamTracking/Protobufs "$CSGO_PROTO_DIR"

### --- Export env vars for CMake ------------------------------------------
export SOURCE2TOOLKIT_SDK="$SOURCE2TOOLKITSDK_DIR"
export S2SDK="$S2SDK_DIR"
export MMSOURCE_DEV="$MMSOURCE_DIR"
export CSGO_PROTO="$CSGO_PROTO_DIR/csgo"

echo "Using SOURCE2TOOLKIT_SDK=$SOURCE2TOOLKIT_SDK"
echo "Using S2SDK=$S2SDK"
echo "Using MMSOURCE_DEV=$MMSOURCE_DEV"
echo "Using CSGO_PROTO=$CSGO_PROTO"

### --- Build ---------------------------------------------------------------
echo "=== Starting build ==="

rm -rf build
mkdir build
cd build

cmake .. -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_C_COMPILER=clang-18 \
  -DCMAKE_CXX_COMPILER=clang++-18

echo "=== Building with Clang | RelWithDebInfo ==="

cmake --build . -j"$(nproc)"