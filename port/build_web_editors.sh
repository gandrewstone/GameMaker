#!/bin/bash
# Builds the GameMaker design tools (menu, utility, the seven editors) and the player as WebAssembly and packs them with the page
# into a static folder (any web server) and a .zip of the same folder.
#
#   bash port/build_web_editors.sh [--out DIR]
#
#     --out DIR   output folder (default: port/web-editors); DIR.zip is written next to it
#
# Needs Emscripten, cmake, ninja and git, as build_web.sh does (see the top of that file). The player is part of it: the
# menu's "Play" runs it on the same page, in the same folder as the editors, so a game can be tested as soon as it is saved.
# GM_WEB_EDITORS_BUILD_DIR changes the build folder (port/build-web-editors).
set -e
here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/.." && pwd)
out="$here/web-editors"
while [ $# -gt 0 ]; do
  case "$1" in
    --out) out=$2; shift 2 ;;
    --out=*) out=${1#--out=}; shift ;;
    -h|--help) awk 'NR>1 && /^#/ { sub(/^# ?/, ""); print; next } NR>1 { exit }' "$0"; exit 0 ;;
    *) echo "build_web_editors.sh: unknown option $1 (try --help)" >&2; exit 2 ;;
  esac
done

if ! command -v emcmake >/dev/null 2>&1; then
  sdk=${EMSDK:-$HOME/emsdk}
  if [ -f "$sdk/emsdk_env.sh" ]; then
    # shellcheck disable=SC1091
    . "$sdk/emsdk_env.sh" >/dev/null 2>&1
  fi
fi
if ! command -v emcmake >/dev/null 2>&1; then
  echo "build_web_editors.sh: Emscripten was not found. Install the emsdk (see build_web.sh) or set EMSDK to its folder." >&2
  exit 1
fi

build=${GM_WEB_EDITORS_BUILD_DIR:-$here/build-web-editors}
programs="gmmenu gmutility gmpalchos gmblocedit gmmonedit gmmapmaker gmcharedit gmimage gmsndedit gmgrator gmplayer"
if [ ! -f "$build/build.ninja" ]; then
  emcmake cmake -S "$here" -B "$build" -GNinja -DCMAKE_BUILD_TYPE=Release -DGM_WEB_TARGET=browser
fi
# shellcheck disable=SC2086
cmake --build "$build" --target $programs

# the page and the programs
rm -rf "$out" "$out.zip"
mkdir -p "$out"
cp "$here/web/editors.html" "$out/index.html"
cp "$here/web/gmedit-web.js" "$out/"
for p in $programs; do
  cp "$build/$p.js" "$build/$p.wasm" "$out/"
done

# what a first visit finds in the store: the help files, the configuration and the sample game (other games: rights unsettled)
src="$repo/runtime/run/GM"
stage=$(mktemp -d)
mkdir -p "$stage/SAMPLE"
cp "$src"/*.HLP "$src/GM.CFG" "$src/CONFIG.DAT" "$stage/"
cp "$src"/SAMPLE/* "$stage/SAMPLE/"
(cd "$stage" && cmake -E tar cf "$out/gmdata.zip" --format=zip -- *)
rm -rf "$stage"

(cd "$out" && cmake -E tar cf "$out.zip" --format=zip -- *)
echo "built $out (and $out.zip)"
ls -l "$out"
