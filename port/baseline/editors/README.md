# Editors oracle: the port's editors against the original DOS editors

The DOS editors are rebuilt from the repository source (Borland C++ in DOSBox, demo flag off) and driven by `KEYS.EXE`, which
hooks INT 16h (keys, paced in BIOS ticks after the previous event), INT 33h (plays the mouse driver: moves and clicks as
cumulative mickey counters) and the timer (non-key events when the program does not poll the keyboard). The port reads the same
script (`GM_KEYSCRIPT`, headless). Both write screen dumps (`D` events) and the files the editor saves; `compare.ps1` compares them.

Order: `build_keys.ps1`, `build_dos_editors.ps1`, `getfont.ps1` (the DOS ROM font for the port, `GM_ROMFONT`; the port's own
font is Unscii), then `runcase.ps1 -Name n -Program palchos -Steps ...` or `run_suite.ps1`. Helpers: `keyscript.ps1` (step
syntax), `mouse.ps1` (absolute mouse positions after sending the cursor to the corner), `dumpview.ps1`, `gridview.ps1`,
`showdiff.ps1`, `compare_pixelsets.ps1` (animated screens: per-pixel colour sets over many dumps).

Things the oracle taught (all in the code comments too):
- raw NUL bytes inside string literals are dropped by modern compilers: the nine sources are fixed with `\000` (`.gitattributes`
  keeps their bytes as they are);
- the first mouse event of a DOS run slams the cursor to the corner (MouseClass::Change, 16-bit wrap): scripts start with a
  priming move; afterwards DOS and the port track the cursor identically;
- animated screens and saved animation state (monster `lasttime`, `cur.pic`) depend on the clock: masks (`-Mask`, `-FileMask`).

Status (2026-10-09): palchos, blocedit, monedit, mapmaker, charedit (menus), sndedit, grator, image, utility, menu: every screen
dump and saved file compared so far is identical (the monster strip and 54 animation bytes are masked), including edits made
with the mouse (palette, pixels, map painting, monster frames, sound bars, scenes, GIF cutting). The character maker's sequence editor (Idle > Define Sequence) was reached too: its preview animates on the clock, and the per-pixel colour sequences over 24 dumps are the same in DOS and the port, shifted by the load time (so it is not a suite case; a static dump differs by a few pixels depending on the phase). The suite lists the keyboard cases and three mouse/flow cases (gam-click, img-cut, chr-flow); the other mouse cases were run by hand.
