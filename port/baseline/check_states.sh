#!/bin/sh
# Replays recordings in gmplay and compares the frame table (tick, score, scene, character position, scroll offset
# of every 20th tick) with the table the DOS run left in the repository (baseline/<name>/states.txt). Unlike
# compare.ps1/compare.sh it needs no DOS run and no DOSBox, so it works anywhere gmplay does; it checks game logic
# and scrolling, not video memory.
#
#   usage: check_states.sh <name>...      name = a game with a shipped demo (bcuda, houses, ...) or <game>-gen for a
#                                         generated recording (donut-gen, glub-gen, ...; baseline/recs/<game>.rec)
#   environment: GMPLAY, FDUMP_BIN (default: ../build/gmplay and ../build/fdump), GM_BUILD_DIR as for build.sh
set -u
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
build=${GM_BUILD_DIR:-$here/../build}
exe=""; if [ -f "$build/gmplay.exe" ]; then exe=.exe; fi
gmplay=${GMPLAY:-$build/gmplay$exe}
fdump=${FDUMP_BIN:-$build/fdump$exe}
work=${TMPDIR:-/tmp}/gm_states_check
export GM_NO_SETTINGS=1
export GM_HEADLESS=${GM_HEADLESS-1}     # no window needed: the replays are deterministic (unset GM_HEADLESS to watch them)
mkdir -p "$work"
bad=0

for name in "$@"; do
  game=${name%-gen}
  table="$here/$name/states.txt"
  if [ ! -f "$table" ]; then echo "$name: no $table"; bad=1; continue; fi
  frames=$(wc -l <"$table" | tr -d ' ')
  run="$work/$name"
  rm -rf "$run"; mkdir -p "$run"
  cp -r "$root/runtime/run/GM" "$run/GM"
  cp -r "$root/cd/gameware/$game" "$run/GM/$game"
  gam=$(cd "$run/GM/$game" && ls | grep -i '\.gam$' | head -1)
  if [ "$name" != "$game" ]; then
    cp "$here/recs/$game.rec" "$run/GM/$game/GEN.REC"; rec=GEN.REC
  else
    rec=$(cd "$run/GM/$game" && ls | grep -i '\.rec$' | head -1)
  fi
  ( cd "$run/GM" && FDUMP="$run/frames.bin" FDUMPMAX=$frames timeout 900 "$gmplay" "$game/$rec" "$game/$gam" >"$run/out.txt" 2>"$run/err.txt" )
  if [ ! -f "$run/frames.bin" ]; then echo "$name: NO RESULT $(head -c 200 "$run/err.txt")"; bad=1; continue; fi
  "$fdump" list "$run/frames.bin" | tr -d '\r' >"$run/states.txt"
  tr -d '\r' <"$table" >"$run/expected.txt"             # the repository copy may have been checked out with CRLF
  if cmp -s "$run/states.txt" "$run/expected.txt"; then
    echo "$name: identical to DOS ($frames frames)"
  else
    echo "$name: DIFFERS from DOS"; diff "$run/states.txt" "$run/expected.txt" | head -6; bad=1
  fi
done
exit $bad
