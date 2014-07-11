#************************************************************************
#									*
# File: 	GM                                                      *
# Description:	Make file for building the game maker executable        *
#									*
#************************************************************************

!include "makers.mak"

chk.exe:        chk.obj\
                startupl.obj\
                oldmouse.obj\
                gma.obj\
                windio.obj\
                scrnrout.obj
                $(LINKER) $(LFLAGS) @chk.lnk
                linkchk.exe

startupl.obj:   startup.asm
                $(ASSEMBLER) /M /MX /Q /T /D__LARGE__ startup.asm, startupl.obj
chk.obj:        chk.c bytenums.h
                $(CCOMPILER) $(CFLAGS) chk.c
gma.obj:        gma.asm
                $(ASSEMBLER) $(AFLAGS) gma.asm
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