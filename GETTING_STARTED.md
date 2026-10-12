# Getting started with GameMaker

GameMaker (1994, Recreational Software Design) lets you build a complete game without writing code. You draw
pictures, group them into animated monsters and characters, paint maps, and wire the maps together. This page is a
short first-hour tour of the tools, in the order you would normally use them.

## 1. Start it

Pick one:

| Way | How |
|---|---|
| **DOSBox** (the original) | Install [DOSBox](https://www.dosbox.com/), `cd` to the repo root, run `dosbox`, then inside it: `mount c runtime`, `c:`, `cd c:\gm`, `gm`. See the [README](README.md). |
| **Browser** (the port) | Build with `bash port/build_web_editors.sh` (needs the Emscripten SDK), host the `port/web-editors/` folder with any web server and open `editors.html`. Your files are kept in the browser, and the page has Zip import/export buttons. |
| **Desktop** (the port) | See [port/README.md](port/README.md) to build; `gmplay` plays games, and the editor targets are in `port/cmake/Editors.cmake`. |

You land on the **main menu strip**: **Play, Design, Utilities, About, Quit**. Move with the mouse or the Left/Right
arrow keys and press Enter. Esc backs out of most screens.

## 2. Play before you make (5 minutes)

Choose **Play** and load a game from the list. Good ones to try:

- `tutor`: not a real game but a guided demo of scrolling, solid blocks, one-way doors, shooting, blocks that change
  on contact or over time, gravity, keys and doors.
- `sample`: the smallest complete game. It is the best one to open later in the editors, because everything in it is
  small enough to understand.
- `houses`, `zark`, `bcuda`, `pipemare`: bigger games showing what the tools can produce.

Arrow keys move the character, Space shoots (in `tutor`). Esc leaves a game.

## 3. How a game is built

A game is a folder of files, each made by one editor. The **Design** menu has one entry per editor:

| Design menu | Makes | File | Think of it as |
|---|---|---|---|
| **Palette** | the 256 colors | `.pal` | your paint box |
| **Block** | small pictures (a wall, a floor, a coin, one frame of a walk) | `.bbl` | background tiles |
| **Monster** | animated objects built from blocks, plus their powers | `.mon` (and `.mbl`) | enemies, items, scenery that moves |
| **Map** | scenes made of blocks and monsters | `.map` | levels / rooms |
| **Character** | the player's animated sequences and powers | `.chr` (and `.cbl`) | the hero |
| **Image** | cuts pictures out of a GIF into a block set | | import your own art |
| **Sound** | sound effects, up to 30 per set | `.snd` | beeps, shots, deaths |
| **Integrator** | joins maps, characters, monsters, and sounds into a game | `.gam` | the level graph |

Work from the bottom of the stack up: colors, then blocks, then monsters and characters, then maps, then the
Integrator. Every editor opens with a prompt to create a new file or load an existing one, and all of them have a
built-in help screen (the original manual covers the details).

## 4. Your first game, step by step (about an hour)

Name everything the same (for example `mygame`) so the files stay together.

### Step 1: Palette
Open **Design > Palette** and create a new palette. Click a color in the bar at the top, then change its
red/green/blue intensities (0 to 63) by clicking a number and typing, or by holding the plus/minus buttons. Drag a
color to another slot with mouse button 2 (press `M` for move, `C` for copy while dragging).
Tips: color 255 shows as clear (transparent) during play, so keep it for the background of characters and monsters.
Save, then quit back to the menu. If you do not want to design colors yet, load `sample.pal` or `tutor`'s palette
and move on.

### Step 2: Blocks
Open **Design > Block** and create a block set that uses your palette. Pick a color with the left mouse button and
another with the right one, then click pixels in the drawing area (hold the button to paint). Useful keys with the
pointer on a block: `E` erase and fill, `F` flip, `R` rotate, `S` swap, `I` info (block attributes). Drag blocks around
the block bar to copy them.
Draw: a floor/wall tile, a background tile, and a few frames for a monster. Block 0 is what a new map is filled with.

### Step 3: Monster
Open **Design > Monster**. Create a monster set, select a monster in the top bar, and drag blocks from the bottom
block bar into the middle sequence bar. The order of the blocks is the animation. Click a monster with mouse
button 2 to open its information screen and give it its powers (how it moves, whether it hurts, what it does on
contact).

### Step 4: Character (the player)
Open **Design > Character**. Pick block(s) from the bar at the top (a character can be one or two blocks tall), put them
in the grid, and repeat for each animation frame, placing each frame a little further in the direction it should
walk. Backspace removes the last frame, the Left/Right arrows step through frames, and the Up/Down arrows change how
long each frame pauses. Then give the character its powers.

### Step 5: Map
Open **Design > Map**. Create a map using your block set. Pick a block or monster in the scroll bar on the right and click
on the map to place it. Switch between the block bar and the monster bar with the buttons, scroll the map with the arrow
keys or the compass in the toolbar, and use the **Z** button to zoom. Build a first level with a floor, a few
obstacles, and one monster.

### Step 6: Sound (optional)
Open **Design > Sound**. The bar chart is time (left to right) against pitch (bottom to top): click, or hold the left
button and draw, to place bars. A bar at the very bottom is silence. Press **Play** to hear it. A sound set holds up to
30 sounds that you later assign to scenes.

### Step 7: Integrator
Open **Design > Integrator**. Click anywhere in the left box to create a **scene** (a small square). Select a scene to
name it and set what it uses: its map, character, monsters, and sound set. Use **Select**, then drag a line from one
scene to another to make a **link**, which is the path the player takes between scenes. Click a link to edit when it
fires. All the files a scene needs must be defined before its link editor opens.
Save the game.

### Step 8: Play it
Back on the main menu, choose **Play** and pick `mygame`. Walk around, fix what feels wrong in the editor that made it,
and play again. That loop (play, change, play) is the whole craft.

## 5. Using your own artwork (Image)

**Design > Image** reads a GIF. Type its path and file name, choose a block set to capture into, then point at the part
of the image you want and press mouse button 1 to capture it. The arrow keys resize the capture window, and mouse
button 2 moves on to the next free block. Save the block set and palette from its menu so your editors can use them.

## 6. Making assets with external tools

You need no special software. GameMaker reads a few plain, old formats that any current editor can produce.

### Images (GIF)

- **Format:** GIF only, for the Image editor. PNG and JPG will not load, so export to GIF.
- **Size:** the reader assumes a 320x200 screen. A bigger GIF is shown at a quarter size and you move a capture
  rectangle over it. For screen-sized art (backdrops, title screens) use exactly 320x200.
- **Colors:** GIFs are at most 256 colors, which matches the 256-color palette. Interlaced GIFs are handled.
- **Tools:** Aseprite, GIMP, Krita, Photoshop or Paint.NET (save as indexed color, 256 colors or fewer, and export as
  GIF); Pixelorama or Piskel in the browser. To batch-convert with ImageMagick:
  `magick in.png -resize 320x200 -colors 256 out.gif`
- **Tile size:** you capture pieces of the GIF into small blocks, so draw tiles on a regular grid.
- **Palette:** the whole game shares one 256-color palette. Plan your art around a shared palette so the colors stay
  consistent.
- **Example art:** `cd/picture/` has ready-made GIFs by category (animals, cars, space, textures, ...).

### Sound effects

- **Built-in:** the Sound editor draws sounds as bar charts (time against pitch), so beeps and zaps need no external
  tool.
- **Digitized sound:** `.VOC` files (Creative Voice File), loaded by name from the working folder. To make one, record
  or edit in Audacity, export WAV as 8-bit mono at a low sample rate (about 8-11 kHz), then convert with SoX:
  `sox in.wav -r 11025 -c 1 -b 8 out.voc`
- **Examples:** `bebeep.voc`, `dead.voc` and `exclnt.voc` in `cd/gameware/sample`.
- **Sound card:** in the original editor, `.VOC` files only play if the Sound Blaster voice driver loads. It is not
  known whether that applies to the port.

### Music

- **Format:** `.CMF` (Creative Music File, FM synth notes and instruments, similar to MIDI). You cannot compose it
  in GameMaker. A CMF has three blocks (header, instruments, music): up to 16 FM instruments and a single track of
  MIDI-style events, played on an OPL2 FM chip. The port emulates that chip, so it plays CMF music and the drawn
  sounds. It has not been checked how many channels or notes the player can handle at once.
- **The 146 Hz rule:** the player rejects any CMF whose header clock is not **146 ticks per second** (`HDRCLK` in
  `SOUND.H`, checked in `SBMUSIC.C`). Ordinary CMF files use a different clock, so they need converting first.
- **The Sound editor's convert option:** it asks for a `*.cmf` file, rescales the header's ticks per beat and every
  delay in the music data to 146 Hz, and writes a new file. The suggested output name is the input name with `GM`
  put in front, in the same folder. It refuses a file that is already at 146 ("does not need conversion") and
  anything that does not start with `CTMF`.
- **Making a CMF:**
  1. Write the music as MIDI in any sequencer (MuseScore, LMMS, Reaper, ...). Keep to one track, fewer than 16
     channels, and the few voices the FM chip has.
  2. Convert MIDI to CMF with `MID2CMF`. A [Vogons thread](https://www.vogons.org/viewtopic.php?p=1280639) says a
     copy in `MID2CMF.ZIP` on retroarchive.org converts correctly and adds instruments; it needed the `drum.ibk` and
     `melody.ibk` instrument banks, and a separate drum bank fixed a problem for one user. There is no maintained
     download page, so check that archive yourself.
  3. Run the Sound editor's convert option on the result to get a 146 Hz file.
  4. Assign it to a scene in the Integrator, which lists `*.cmf` files when it asks for music.
- **Quickest start:** reuse the 12 tracks in `cd/music/`, which are already in the right form.
- **Checking a file offline:** the port's `port/tools/cmf2wav.cpp` renders a CMF through the real music code to a
  WAV, so you can listen without starting the game. It has not been checked whether it builds on its own.
- **Expect differences:** an [OpenMPT bug report](https://bugs.openmpt.org/view.php?id=1948) says a CMF converted to
  MIDI sounds completely different, and the same is likely the other way, so tune instruments by ear.
- **More on the format:** the [Modding Wiki](https://moddingwiki.shikadi.net/wiki/CMF_Format) and a
  [text description](https://bearstrong.net/tekst97/data/programmering/pcgpe10/cmf/). `CMF2MID` converts the other
  way, to format 0 MIDI.

### Workflow

1. Draw in Aseprite or GIMP at 320x200, indexed color, and export GIF.
2. Capture the pieces into blocks with the Image editor.
3. Draw sounds in the Sound editor, or record in Audacity and convert to 8-bit mono VOC.
4. For music, reuse the CMF files in `cd/music/`, or write MIDI, convert it to CMF, then convert it to 146 Hz in the
   Sound editor.

## 7. Housekeeping

- **Utilities** has the file management and configuration tasks, and a **Transfer** tool that packages a game for
  distribution on disks.
- Copy a game folder in `cd/gameware` (for example `sample`) to start from a working base instead of nothing.
- Back up your work often. The original tools have no undo for most operations.

## 8. Where to go next

- Open the shipped games in the editors and see how they are made. `sample` and `tutor` are the easiest.
- The editors' built-in help screens and the original help files in `cd/gm/*.hlp`.
- [port/README.md](port/README.md) for the modern port and [port/EDITORS_PLAN.md](port/EDITORS_PLAN.md) for the
  editor/web work.
