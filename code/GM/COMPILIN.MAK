!include "newmouse.mak"
startupl.obj:   startup.asm
                $(ASSEMBLER) /M /MX /Q /T /D__LARGE__ startup.asm, startupl.obj
sndedit.obj:    sndedit.c
                $(CCOMPILER) $(CFLAGS) sndedit.c
soundc.obj:     soundc.c
                $(CCOMPILER) $(CFLAGS) soundc.c
windio.obj:     windio.c
                $(CCOMPILER) $(CFLAGS) windio.c
genc.obj:       genc.c
                $(CCOMPILER) $(CFLAGS) genc.c
palc.obj:       palc.c
                $(CCOMPILER) $(CFLAGS) palc.c
pala.obj:       pala.asm
                $(ASSEMBLER) $(AFLAGS) pala.asm
gena.obj:       gena.asm
                $(ASSEMBLER) $(AFLAGS) gena.asm
scrnrout.obj:   scrnrout.asm
                $(ASSEMBLER) $(AFLAGS) scrnrout.asm
oldmouse.obj:   oldmouse.asm
                $(ASSEMBLER) $(AFLAGS) oldmouse.asm
jsticka.obj:    jsticka.asm
                $(ASSEMBLER) $(AFLAGS) jsticka.asm
jstickc.obj:    jstickc.c
                $(CCOMPILER) $(CFLAGS) jstickc.c
facelift.obj:   facelift.c
                $(CCOMPILER) $(CFLAGS) facelift.c
graphc.obj:     graphc.c
                $(CCOMPILER) $(CFLAGS) graphc.c

