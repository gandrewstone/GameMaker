#************************************************************************
#                                                                       *
# File:   GM                                                            *
# Description:	Make file for building the game maker executable        *
#                                                                       *
#************************************************************************

!include "makers.mak"

register.exe:   register.obj\
                startupl.obj\
                oldmouse.obj\
                windio.obj\
                gma.obj gen.h\
                scrnrout.obj
                $(LINKER) $(LFLAGS) @register.lnk

startupl.obj:   startup.asm
                $(ASSEMBLER) /M /MX /Q /T /D__LARGE__ startup.asm, startupl.obj
register.obj:   register.c
                $(CCOMPILER) $(CFLAGS) register.c
gma.obj:        gma.asm
                $(ASSEMBLER) $(AFLAGS) gma.asm
oldmouse.obj:   oldmouse.asm
                $(ASSEMBLER) $(AFLAGS) oldmouse.asm
windio.obj:     windio.c
                $(CCOMPILER) $(CFLAGS) windio.c
genc.obj:       genc.c
                $(CCOMPILER) $(CFLAGS) genc.c
palc.obj:       palc.c
                $(CCOMPILER) $(CFLAGS) palc.c
jstickc.obj:    jstickc.c
                $(CCOMPILER) $(CFLAGS) jstickc.c
scrnrout.obj:   scrnrout.asm
                $(ASSEMBLER) $(AFLAGS) scrnrout.asm
jsticka.obj:    jsticka.asm
                $(ASSEMBLER) $(AFLAGS) jsticka.asm

