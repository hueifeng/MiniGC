#!/usr/bin/env bash
# Build MiniGC (the standalone GC) and run GcDemo with it loaded.
#
# Works on macOS and Linux. On macOS the produced shared library is
# libMiniGC.dylib; on Linux it is libMiniGC.so. CoreCLR's DOTNET_GCPath
# accepts either, so this script picks up whichever CMake produced.
#
# Without this script (or unset DOTNET_GCPath), GcDemo uses CoreCLR's
# in-box GC. Use that to compare "normal" runtime behaviour with the
# custom-GC path that this project enables.
set -euo pipefail

if ! command -v dotnet >/dev/null 2>&1; then
  echo "error: 'dotnet' not on PATH. Install the .NET 10 SDK first." >&2
  exit 127
fi
if ! command -v cmake >/dev/null 2>&1; then
  echo "error: 'cmake' not on PATH. Install CMake 3.20+." >&2
  exit 127
fi

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_dir="$script_dir"
build_dir="$repo_dir/MiniGC/build"
demo_dir="$script_dir/GcDemo/GcDemo.csproj"

echo ">>> Building MiniGC ..."
cmake -S "$repo_dir/MiniGC" -B "$build_dir" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$build_dir" --parallel

# Pick up whichever artifact the host platform produced.
if [ -f "$build_dir/libMiniGC.dylib" ]; then
  gc_path="$build_dir/libMiniGC.dylib"
elif [ -f "$build_dir/libMiniGC.so" ]; then
  gc_path="$build_dir/libMiniGC.so"
else
  echo "error: MiniGC shared library was not produced under $build_dir" >&2
  ls -la "$build_dir" >&2 || true
  exit 1
fi

echo ">>> Using $gc_path"
echo ">>> Building GcDemo ..."
dotnet build "$demo_dir" -c Debug

echo ">>> Running with DOTNET_GCPath=$gc_path"
echo "    (unset DOTNET_GCPath to use CoreCLR's in-box GC instead)"
echo "------------------------------------------------------------"
DOTNET_GCPath="$gc_path" \
  dotnet run --project "$demo_dir" --no-build --no-launch-profile "$@"
