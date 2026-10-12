# Editors survey (Phase 0)

Source: static reading of `code/GM/*.MAK`, `*.C`, `*.CPP`, `*.ASM` and `port/`. Nothing was compiled or run yet, so
"works" below means "the code path exists in the port", not "tested".

## 1. Programs and how they chain

`GM.EXE` (GM.ASM, tiny-model asm) is the launcher. It holds a table of program names and loads them with DOS
EXEC (`LoadExecDataBlock`). The programs return a `QuitCodes` value as their exit code and GM runs whichever
the exit code names (`palchos` ... `grator`, `utility`, `playgame`, `menu`, `quit`). `MENU.EXE` is the top bar
(Play / Design / Utilities / About / Quit); it picks the next program with `exit(code)` and is re-entered with the
previous program as `argv[2][0]-'A'`. `CHAIN.C` is a spawn helper that is not part of the editor set.

So the process model is "run one program, read its exit code, run the next". No editor spawns another itself. That
maps directly to the planned web shell: each program is a wasm module, and its exit code drives the next module.
`GM.ASM` is replaced by JS; `MENU.C` needs only a port of its `exit()`/argv contract.

The registration check (`CHKREG`, `OKMSG` argv[1]) is compiled out of `MENU.C` unless `CHKREG` is defined; the
other editors get no such arguments.

## 2. Link sets (from the .MAK files)

Common to every graphical editor: `genc windio facelift graphc scrnrout pala gena jsticka palc newmouse`.

| Program | Own sources | Additional modules |
|---|---|---|
| palchos | palchos | genclass gasclass geninput timer windclss findfile |
| blocedit | blocedit | blocc bloca findfile genclass gasclass geninput timer windclss |
| mapmaker | mapmaker | mapc blocc bloca clrbloca genclass gasclass geninput timer windclss |
| monedit | monedit | blocc bloca genclass gasclass geninput timer windclss |
| charedit | charedit | blocc bloca soundc jstickc (no genclass set) |
| image | image | gifc blocc bloca genclass gasclass geninput timer windclss |
| grator | grator gramap | mapc blocc bloca findfile genclass gasclass geninput timer windclss |
| sndedit | sndedit | soundc jstickc genclass gasclass geninput timer windclss |
| utility | utility | **oldmouse** (not newmouse) gma findfile jsticka jstickc |
| menu | menu | **oldmouse** gma windio findfile genc scrnrout |

Two mouse stacks: `utility` and `menu` use `OLDMOUSE.ASM` (int 33h wrapper), the eight graphical editors use
`NEWMOUSE.ASM` (interrupt-driven handler plus timer hook).

## 3. What the player port already covers

Already compiled in `engine_objs` for `gmplay`, so the editors can reuse them as they are:
`WINDIO.C GENC.C GIFC.C PALC.C GRAPHC.C FINDFILE.CPP JSTICKC.C SBMUSIC.C`, plus the asm replacements
`gfx_asm.cpp` (Point, GetCol, BoxFill, Gwritestr, GetROMFont, drawblk, getblk, Palette, SetAllPal, SetAllPalTo,
GetAllPal), `text_asm.cpp` (SCRNROUT: text-mode windows, writestr, boxes), `input_asm.cpp` (keyboard handling,
JSTICKA, OLDMOUSE with 640x200 virtual coordinates), and `vgaobj.cpp` (VGA `VideoMode` object).

`dosplat.c` provides: `gm_mouse_show/get/set` (pointer from raylib, scaled to the 640x200 virtual space), a small
arrow cursor drawn over the image, text-mode render with a soft cursor cell, `gm_int86` (int 10h, 16h, ...; int
33h answers "no driver, replaced natively"), `gm_bioskey`, `MK_FP` over a flat DOS-memory buffer, `farmalloc`,
`farcoreleft`, `findfirst`/`findnext` (`dosfind.c`), case-insensitive DOS file names (`dospath.c`).

Consequence: `menu` and `utility` look close to buildable now. The Windows/Linux port builds with `-include
gmcompat.h`; their only new need is the shell logic of section 5.

## 4. Missing pieces, by module

| Gap | Used by | Notes |
|---|---|---|
| **NEWMOUSE.ASM** (379 lines): `MouseInterrupt`, `SetMouseRoutine`, `NewTimer`/`OldTimer`/`Clock`, `Low256*` | all 8 graphical editors | `Clock` is read directly by blocedit, charedit, mapmaker, monedit, blocc (animation timing); the mouse handler calls `MouseMoved` and `KeyUpdate` (`DISP`-style hooks) in `GENINPUT`/`WINDCLSS`. Needs a C++ replacement with `Clock` driven from the same ~18.2 Hz or 35 Hz timer the player uses (confirm which). `Low256*` look like `VideoMode` entries that `vgaobj.cpp` may already implement: check for duplicates. |
| **BLOCA.ASM** (`drawblk`, `getblk`) | blocedit, mapmaker, monedit, charedit, image, grator | `gfx_asm.cpp` already has `drawblk`/`getblk`; verify they are the BLOCA versions and not the GENA ones. |
| **CLRBLOCA.ASM** (`drawcbloc`) | mapmaker | 76 lines, not ported. |
| **MEMBLOCA / GENBUFA / SVGAA / DISP** | not in any editor link set | Player-only or SVGA code; skip. |
| **GMA.ASM** (`Palette`) | utility, menu | 36 lines; `gfx_asm.cpp` already defines `Palette`. |
| **GENCLASS / GASCLASS / GENINPUT / TIMER / WINDCLSS (.CPP)** | palchos, blocedit, mapmaker, monedit, image, grator, sndedit | Plain C++ (classes, linked lists, input dispatch). Only `TIMER.CPP` touches hardware: it saves and replaces the user-timer vector INT 1Ch with `NewTimer`, which increments `Clock`. The port already has `gm_setvect`/`gm_getvect` and a PIT-driven tick (18.2 Hz default) in `dosplat.c`, so the hook should work once `NewTimer` exists. Needs a compile pass. |
| **BLOCC.C, MAPC.C, GRAMAP.C, SOUNDC.C** | see table | Plain C. `SOUNDC.C` reads `BLASTER` and loads the SB driver through `MK_FP(driversegment,0)`; this needs a "no sound card" path or a hookup to `sound_port.cpp`. |
| **MKFP / 0xB800 text buffer** | GENC | Text mode already covered by dosplat. |

## 5. DOS API use in the editor sources

Counted by grepping the editor files only (excluding the player-only modules). Most of the 15k lines are calls that
the port already maps.

- **`bioskey(0/1/2)`**: everywhere (UI loops). Covered by `gm_bioskey`; `bioskey(2)` (shift state) is used by blocedit.
  returns 3 while shift is held, which satisfies the `&0x03` tests. Ctrl/Alt bits are not reported; re-check when
  testing the editors' other hotkeys.
- **`farmalloc` / `farfree` / `farcoreleft`**: blocedit (`blkstruct` arrays of size `BACKBL+1`), charedit, monedit,
  mapmaker, image, grator, palc, dirtrect. `image` calls `farmalloc(BLen=farcoreleft())`, i.e. it grabs all remaining
  far memory. `gm_farcoreleft()` returns the free bytes of the port's flat arena (`ARENA_BASE..ARENA_END` in `dosplat.c`,
  0x10000..0xA0000 = 576 KB), so `image` would take all of it in one block and every later `farmalloc` would fail.
  Needs a cap or a smaller request. The 576 KB arena itself must also be checked against the largest editor
  (blocedit's `blkstruct` arrays). **Risk item.**
- **`int86(0x10,...)`**: only mode switches (`TMODE`) in `GENC.C`; covered.
- **`MK_FP(0xA000,0)` / `0xB800`**: screen save/restore in `GENC.C`; covered by the flat buffer.
- **`findfirst` / `FindFiles`**: file pickers in `WINDIO.C` and `UTILITY.C` (`FA_DIREC|FA_ARCH`); covered by
  `dosfind.c`. Directory listing and `WorkDir` handling need the project tree to exist before first launch.
- **`getdisk`** (`FULLPATH.C`) is not linked into any editor.
- **Spawning and `exec`**: none in the editors. Only `CHAIN.C` and GM.ASM.
- **`getenv("BLASTER")` / `SOUND`**: `SOUNDC.C`; missing env means no card.
- **No EMS/XMS use** in the editor sources (grep found nothing). The "EXTRA MEMORY" line in blocedit is a print of
  `farcoreleft()`/`coreleft()`.
- **No direct port I/O** (`inportb`/`outportb`) in the editor files; all hardware access is in the asm modules.

## 6. Out of scope (confirmed)

`INSTALL`, `FLOPINST`, `MKDSK`, `REGIST`/`REGISTER`/`ADDREG#`/`TELLREG#`, `CHK`/`LINKCHK`, `UPDATE`, `XFERPLAY`,
`SWAPPAL`, the SB driver binary `SNDBLAST.DRV`, hardware recording in `sndedit`. `PLAYGAME` is already the web player.

## 7. Risks, ranked

1. **NEWMOUSE port** (timer `Clock`, mouse callback, key events) is the one real unknown. Every graphical editor
   depends on it. Timing semantics (`Clock` rate) decide whether animation previews run at the right speed.
2. **Memory size assumptions** (`image` taking all of `farcoreleft()`, big `blkstruct` arrays). Size the flat DOS
   memory buffer for the largest editor and cap `farcoreleft()`.
3. **Mouse input model**: editors poll `moustats` in tight loops (`while(!bioskey(1)&&!buts)`). In the browser, these
   loops must yield. `gm_os_yield` exists for the player, so this is tuning, not new design.
4. **Module switching**: one wasm per program means the DOS file store must be shared. The player's IDBFS mount
   already persists saves; the switch must `syncfs` before tearing down.
5. **Ctrl/Alt key reporting**: `gm_bioskey(2)` only reports shift. Fine for blocedit; check the other editors' hotkeys.
6. **Help files**: `DisplayHelpFile()` reads `*.HLP` from the current directory. They must be mounted read-only.

## 8. Findings that change the plan

- `menu` and `utility` do not need `NEWMOUSE`. They are the cheapest to bring up first, to prove the shell, the file
  store and module switching without the mouse timer risk.
- `palchos` is still the right first graphical editor, but the first milestone should be **`utility` + `menu`**
  (exit-code chaining and file pickers), then `palchos` (NEWMOUSE).
- Five of the six missing asm pieces are already replaced or are not linked; the genuine porting job is
  `NEWMOUSE.ASM` plus `CLRBLOCA.ASM`.

## 9. Next steps (Phase 1, in order)

1. Add a native desktop build target `gmedit-<name>` for `menu`, `utility`, `palchos` using the same shim, so problems
   can be debugged natively before wasm.
2. Write `newmouse_port.cpp` replacing `NEWMOUSE.ASM` (and `CLRBLOCA` in C++).
3. Cap the `farcoreleft()`-sized allocation in `IMAGE.C` and check the 576 KB arena against the largest editor.
4. Make the program table / exit-code shell (native first, JS later).
5. Wasm builds per editor, then the browser shell.

## 10. Corrections found while building (Phase 1)

- `TRANMOUS.HPP` (included by every graphical editor's main file) defines `initmouse`, `moucur`, `setmoupos`, `moustats`,
  `Scrn`, `Cur` and `Time`; `NEWMOUSE.ASM` only provides `MouseInterrupt`, the timer tick and the row routines
  (`Low256Point/GetCol` live in `WINDCLSS.CPP`). Section 4 listed more as missing than was.
- `MouseClass::Change()` cannot be fed with mickeys; the port moves `Cur` directly (see `newmouse_port.cpp`).
- Inline `asm` is in `GENINPUT.CPP`, `WINDCLSS.CPP` and `SOUNDC.C` (5 blocks) only.
- `GMGEN.H` defines `CRIPPLEWARE`: the sources are the demo configuration. Turned off for the port.
- `MOUSE` is a global switch of the original build (MOUSEFN.H); menu and utility use `OLDMOUSE` but are compiled without it.
