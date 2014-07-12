!include "d:\makers.mak"

viddrv.lib:     viddrvc.obj viddrva.obj
                del viddrv.lib
                $(LIBRARIAN) viddrv.lib $(LIBFLAGS) +viddrvc.obj +viddrva.obj

viddrva.obj:    viddrva.asm
                $(ASSEMBLER) $(AFLAGS) viddrva.asm

viddrvc.obj:    viddrvc.cpp
                $(CCOMPILER) $(CFLAGS) viddrvc.cpp



