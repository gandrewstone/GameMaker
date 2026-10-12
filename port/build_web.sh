#!/bin/bash
# One step from the source to a web-ready folder: builds the player as WebAssembly and packs it with the page and the
# games you choose. The result is a static site (any web server) and a .zip of the same folder.
#
#   bash port/build_web.sh [--games LIST | --standalone GAME] [--out DIR]
#
#     --games LIST   games to pack in: names separated by commas (sample,houses), "all" (every GameMaker 3.0 game in
#                    cd/gameware) or "none". Default: sample. Players can always drop their own game folders on the page.
#     --standalone GAME
#                    instead of a site: ONE html file with the player and that one game inside (GAME.html). It opens
#                    from disk with a double click (no web server, no other files) and starts with a Play button.
#     --out DIR      output folder (default: port/web-dist, or port/web-standalone with --standalone); DIR.zip is
#                    written next to it
#
# Needs Emscripten (https://emscripten.org/docs/getting_started/downloads.html), cmake and git (CMake fetches raylib
# and ymfm). The emsdk is taken from the current shell if `emcmake` is on the PATH, otherwise from $EMSDK or ~/emsdk:
#
#     git clone https://github.com/emscripten-core/emsdk.git ~/emsdk && cd ~/emsdk && ./emsdk install latest && ./emsdk activate latest
#
# On Windows run it from WSL or Git Bash with an emsdk there. GM_WEB_BUILD_DIR changes the build folder (port/build-web).
set -e
here=$(cd "$(dirname "$0")" && pwd)
games=sample
standalone=""
out=""

while [ $# -gt 0 ]; do
  case "$1" in
    --games) games=$2; shift 2 ;;
    --games=*) games=${1#--games=}; shift ;;
    --out) out=$2; shift 2 ;;
    --out=*) out=${1#--out=}; shift ;;
    --standalone) standalone=$2; shift 2 ;;
    --standalone=*) standalone=${1#--standalone=}; shift ;;
    -h|--help) awk 'NR>1 && /^#/ { sub(/^# ?/, ""); print; next } NR>1 { exit }' "$0"; exit 0 ;;
    *) echo "build_web.sh: unknown option $1 (try --help)" >&2; exit 2 ;;
  esac
done

case "$standalone" in
  *,*) echo "build_web.sh: --standalone takes one game (a single file holds one game); use --games for several." >&2; exit 2 ;;
esac
if [ -n "$standalone" ]; then games=$standalone; fi        # the check below then applies to it
if [ -z "$out" ]; then
  if [ -n "$standalone" ]; then out="$here/web-standalone"; else out="$here/web-dist"; fi
fi

if ! command -v emcmake >/dev/null 2>&1; then
  sdk=${EMSDK:-$HOME/emsdk}
  if [ -f "$sdk/emsdk_env.sh" ]; then
    # shellcheck disable=SC1091
    . "$sdk/emsdk_env.sh" >/dev/null 2>&1
  fi
fi
if ! command -v emcmake >/dev/null 2>&1; then
  echo "build_web.sh: Emscripten was not found. Install the emsdk (see the top of this file) or set EMSDK to its folder." >&2
  exit 1
fi

# Check the game names before the long part: only GameMaker 3.0 games in cd/gameware can be packed (their .gam file
# starts with "GM"). The shareware games in cd/sharware use an older format and come with their own program; neither this
# player nor the original DOS one can run them.
repo=$(cd "$here/.." && pwd)
is_v3_game() {   # $1 = game folder
  for f in "$1"/*.gam "$1"/*.GAM; do
    if [ -f "$f" ]; then [ "$(head -c 2 "$f")" = "GM" ]; return; fi
  done
  return 1
}
working=""
for d in "$repo"/cd/gameware/*/; do
  if is_v3_game "$d"; then working="$working $(basename "$d")"; fi
done
case "$games" in
  all|none) ;;
  *)
    for g in $(printf '%s' "$games" | tr ',' ' '); do
      if is_v3_game "$repo/cd/gameware/$g"; then continue; fi
      if [ -d "$repo/cd/gameware/$g" ] || [ -d "$repo/cd/sharware/$g" ]; then
        echo "build_web.sh: '$g' is in the older GameMaker format (it came with its own player program), which this player cannot run." >&2
      else
        echo "build_web.sh: there is no game '$g' in cd/gameware." >&2
      fi
      echo "Games that work:$working" >&2
      exit 2
    done ;;
esac

build=${GM_WEB_BUILD_DIR:-$here/build-web}
gen=""
if command -v ninja >/dev/null 2>&1; then gen="-G Ninja"; fi
list=$(printf '%s' "$games" | tr ',' ';')

emcmake cmake -S "$here" -B "$build" $gen -DCMAKE_BUILD_TYPE=Release -DGM_WEB_TARGET=browser \
  "-DGM_WEB_GAMES=$list" "-DGM_WEB_OUT=$out" "-DGM_WEB_STANDALONE=$standalone"
cmake --build "$build" --target web_dist

echo
echo "Ready: $out"
echo "       $out.zip"
if [ -n "$standalone" ]; then
  echo "Play it: open $out/$standalone.html in a browser (double-click; no server needed)"
else
  echo "Try it:  cd \"$out\" && python3 -m http.server 8000   then open http://localhost:8000/"
fi
