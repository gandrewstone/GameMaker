!include "makers.mak"

gm.exe: gm.obj
        $(LINKER) $(LFLAGS) gm.obj

gm.obj: gm.asm
        $(ASSEMBLER) $(AFLAGS) gm.asm
