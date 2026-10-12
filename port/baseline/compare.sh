#!/bin/sh
# compare.ps1 for Linux and macOS: replays each game's recording in gmplay and compares the result with a DOS run
# made on Windows (run_baseline.ps1 leaves <game>.bin and <game>.trace.txt in %TEMP%\gm_oracle; copy that folder).
#
#   usage: compare.sh <oracle dir> <game>[:<max frames>] ...
#   environment: GMPLAY, FDUMP_BIN (default: ../build/gmplay and ../build/fdump), GM_BUILD_DIR as for build.sh,
#                RECS=<dir> TAG=<suffix> to replay generated recordings (<dir>/<game>.rec) instead of the shipped demos
set -u
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
build=${GM_BUILD_DIR:-$here/../build}
exe=""; if [ -f "$build/gmplay.exe" ]; then exe=.exe; fi
gmplay=${GMPLAY:-$build/gmplay$exe}
fdump=${FDUMP_BIN:-$build/fdump$exe}
oracle=$1; shift
tag=${TAG:-}
work=${TMPDIR:-/tmp}/gm_port_cmp
mkdir -p "$work"
export GM_NO_SETTINGS=1
export GM_HEADLESS=${GM_HEADLESS-1}     # no window needed: the replays are deterministic (unset GM_HEADLESS to watch them)

CR=$(printf '\r')
# the per-tick trace lines (they start with a digit), without the wall-clock "timer=" field and without CRs
strip_trace() { tr -d "$CR" <"$1" | grep -E '^[0-9]' | sed 's/ timer=[0-9]*//'; }

printf '%-10s %-24s %-30s %s\n' game 'ticks (DOS vs port)' 'logic (frames differing)' 'pixels (same-pan frames)'
for spec in "$@"; do
  g=${spec%%:*}
  max=5000; case "$spec" in *:*) max=${spec#*:};; esac
  run="$work/$g$tag"
  rm -rf "$run"; mkdir -p "$run"
  cp -r "$root/runtime/run/GM" "$run/GM"
  cp -r "$root/cd/gameware/$g" "$run/GM/$g"
  gam=$(cd "$run/GM/$g" && ls | grep -i '\.gam$' | head -1)
  if [ -n "${RECS:-}" ]; then
    cp "$RECS/$g.rec" "$run/GM/$g/GEN.REC"; rec=GEN.REC
  else
    rec=$(cd "$run/GM/$g" && ls | grep -i '\.rec$' | head -1)
  fi
  ( cd "$run/GM" && FDUMP="$run/frames.bin" FTRACE="$run/trace.txt" FDUMPMAX=$max \
      timeout 900 "$gmplay" "$g/$rec" "$g/$gam" >"$run/out.txt" 2>"$run/err.txt" )
  if [ ! -f "$run/frames.bin" ]; then printf '%-10s NO RESULT (%s)\n' "$g" "$(head -c 200 "$run/err.txt")"; continue; fi

  strip_trace "$oracle/$g$tag.trace.txt" >"$run/dos.t"
  strip_trace "$run/trace.txt" >"$run/port.t"
  a=$(wc -l <"$run/dos.t" | tr -d ' '); b=$(wc -l <"$run/port.t" | tr -d ' ')
  bad=$(diff "$run/dos.t" "$run/port.t" | grep -c '^<')
  if [ "$a" = "$b" ] && [ "$bad" = 0 ]; then ticks="identical ($a)"; else ticks="$bad differ; $a vs $b"; fi

  d=$("$fdump" diff "$oracle/$g$tag.bin" "$run/frames.bin" | tr -d "$CR")
  logic=$(echo "$d" | sed -n 's/^of [0-9]* frames: \([0-9]*\) differ in logic.*/\1/p')
  frames=$(echo "$d" | sed -n 's/^frames: \([0-9]*\) vs \([0-9]*\).*/\1 vs \2/p')
  same=$(echo "$d" | sed -n 's/.*identical state and pan (\([0-9]*\) frames): \([0-9]*\) differ.*/\2 of \1/p')
  printf '%-10s %-24s %-30s %s\n' "$g" "$ticks" "$logic of $frames frames" "$same"
done
