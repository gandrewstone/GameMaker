# Verifying the port against DOS

The port is checked against the original engine, not against opinions about how it should behave.
`code/GM/PLAYGAME.C` can be built with `-DFRAMEDUMP`; that build replays a game's shipped `demo.rec`
with no menus and records what happened. The same hook is compiled into the port (`gmplay`), so both
sides write identical records and `compare.ps1` diffs them.

## Result

Everything compared is **identical** between DOS and the port: tick traces, game state and all video memory dumps.

Shipped demo recordings (`compare.ps1 -Games ...`), each replayed to the end of its recording:

| game | ticks (DOS vs port) | frames with differing game state | frames with differing pixels |
|---|---|---|---|
| bcuda | identical (1625) | 0 of 81 | 0 of 81 |
| houses | identical (1049) | 0 of 52 | 0 of 52 |
| nebula | identical (30021) | 0 of 1500 | 0 of 1500 |
| peach | identical (51848) | 0 of 2592 | 0 of 2592 |
| pipemare | identical (3562) | 0 of 178 | 0 of 178 |
| tutor | identical (4753) | 0 of 237 | 0 of 237 |
| volume | identical (1858) | 0 of 92 | 0 of 92 |
| zark | identical (1546) | 0 of 77 | 0 of 77 |

(nebula's recording is much longer than the others; it is compared up to a 1,500-frame cap, `-MaxFrames 1500`.)

Generated recordings: games without a demo were played with seeded random keys (`gen_recordings.ps1`), which makes the
engine save its own recording; those were then replayed in both (`-Recs recs -Tag -gen`):

| game | ticks | frames differing (state / pixels) |
|---|---|---|
| donut | identical (928) | 0 of 46 / 0 of 46 |
| glub | identical (1281) | 0 of 64 / 0 of 64 |
| heart | identical (1206) | 0 of 60 / 0 of 60 |
| outerlim | identical (1281) | 0 of 64 / 0 of 64 |
| penguin | identical (1455) | 0 of 72 / 0 of 72 |
| rings5 | identical (1465) | 0 of 73 / 0 of 73 |
| sample | identical (1465) | 0 of 73 / 0 of 73 |
| terrain | identical (1165) | 0 of 58 / 0 of 58 |

warrior produced no recording (its session never reached the high-score screens in three tries). rings5 first differed
at tick 849 and led to the 16-bit overflow fix described in ../README.md.

"Ticks identical" means a per-tick trace (sequence, animation frame, position, scene, hit points,
lives, score, a checksum of every monster, sub-block position, gravity) matches line for line.
"Pixels" are the 64,000 bytes of video memory every 20 ticks, compared exactly.

### Other platforms

The same replays were run on Linux (Debian, GCC 14, under WSL) with `GM_HEADLESS=1` (no window or audio device) and
compared with the DOS runs made on Windows using `compare.sh`: **all 8 shipped demos and all 8 generated recordings have
identical ticks and identical video memory in every compared frame** (nebula up to 1,500 frames, as above).
`check_states.sh` reproduces the DOS frame tables kept in the repository for the 12 recordings that have one. Windows
was re-run after the portability changes with the same result (16 of 16). The DOS build was also rebuilt from the
modified sources with Borland C++ and still produces the same table (houses).

## How a run works

`playgame <game>\demo.rec <game>\<game>.gam` (instrumented build only) plays the recording and appends
to the files named by environment variables:

| variable | content |
|---|---|
| `FDUMP` | one record per 20 ticks: `"FRM1"`, tick, timer, score, scene, character x/y, scroll offset, 768-byte palette, 64,000 video bytes (read with `tools/fdump.c`) |
| `FTRACE` | one text line per tick (see above) |

## Making replays repeatable

DOS is not deterministic on its own (wall-clock time, uninitialised memory), so the instrumented builds
(`FRAMEDUMP`, both DOS and port) pin down everything that would otherwise leak into a replay:

- **Clock.** While a scene's main loop runs, the timer interrupt stops advancing time and the loop advances it
  by exactly one game tick per pass. Each scene starts from the clock value the previous one ended with, not from
  however long loading took.
- **Random numbers.** The port uses Borland's own `rand()` (a 32-bit LCG, from `tools/BORLANDC/CRTL/CLIB/RAND.C`)
  and `random()` macro. Both builds seed with 1 at start and re-seed with `1 + scene` at the start of each scene, so title
  and fade code that consumes random numbers a timing-dependent number of times cannot shift the sequence.
- **Video.** The oracle is forced onto the software-scroll video path, the only one the port implements.
- **Two original bugs made well defined.** Both are undefined behaviour in the 1994 code whose effect depended on
  leftover memory:
  1. A monster on the last map row makes the engine read `map[100][x]`, one row past the array. In DOS that is the
     start of the data segment (the Borland banner text and some pointers), whose bytes look like monster indices, so
     monsters collided with phantom monsters; which bytes it saw changed whenever the executable's layout changed.
     The instrumented builds and the port give `map` a real extra row that means "no monster".
  2. The "no block here" sentinel entry at the end of each block array (`blks[0][150]` ...) is never initialised, so
     its `solid` flags were leftover heap bytes (one phantom wall in `tutor`). It is zeroed.

  Neither change applies to a normal DOS build.

## Files

| Path | Contents |
|---|---|
| `<game>/states.txt` | the DOS frame table: one line per recorded frame (tick, score, scene, character x/y, scroll offset) |
| `<game>/frameNNN.png` | first, middle and last recorded DOS frame |
| `build_oracle.ps1` | builds the instrumented DOS `PLAYGAME.EXE` from the repo source, in DOSBox with the bundled Borland toolchain (in a scratch copy; tracked `.OBJ`/`.EXE` files are not touched) |
| `run_baseline.ps1` | replays demos in DOSBox and writes the files above; full dumps and traces stay in `%TEMP%\gm_oracle` |
| `compare.ps1` | replays the same demos in `gmplay` and compares ticks, game state and pixels against the DOS run |
| `../tools/fdump.c` | `fdump list|png|diff` for the frame dumps |
| `compare.sh` | `compare.ps1` for Linux and macOS: replays in `gmplay` and compares with a DOS run made on Windows (copy `%TEMP%\gm_oracle`): `compare.sh <oracle dir> bcuda houses ...` |
| `check_states.sh` | a check that needs no DOS run: replays recordings and compares the frame table with the DOS one kept in the repository (`<game>/states.txt`, `<game>-gen/states.txt`). Game logic and scrolling, not pixels. Works on any platform, headless: `check_states.sh bcuda houses donut-gen ...` |
| `gen_recordings.ps1` | plays games with seeded random keys so the engine saves a recording (`recs/<game>.rec`, kept in the repo); `run_baseline.ps1`/`compare.ps1` replay them with `-Recs <dir> -Tag -gen`. A recording is saved only if the session ends cleanly through the high-score screens, which depends on timing: re-run with another `-Seed` for games that failed |
| `<game>-gen/` | the same artifacts for generated recordings |
| `../smoke.ps1`, `../datacheck.ps1` | not DOS comparisons: start every playable game with scripted input and report crashes/frozen screens; validate the game data files |

Reproduce: `port\build.bat`, `baseline\build_oracle.ps1`, `baseline\run_baseline.ps1 -Games bcuda,nebula,...`,
`baseline\compare.ps1 -Games bcuda,nebula,...` (four games at a time is fine; running all eight windows at once
made one of them fail to start).

## Interactive checks

`gmplay` has test hooks for driving it without a person: `GM_TYPE` scripts keystrokes (`\n` Enter, `\b` Backspace,
`\l \r \u \d` arrows, `\hNN<key>` holds a key NN x 80 ms, `\wNN` waits), and `GM_SHOT=<file.png>` writes the exact
emulated screen and exits (`%d` in the name plus `GM_SHOT_EVERY` takes a numbered series). Together they were used
to walk the real menus (game picker, title, main menu), start a game and steer the character with held arrow keys.
