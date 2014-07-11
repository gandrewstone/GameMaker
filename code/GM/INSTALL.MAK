#************************************************************************
#									*
# File: 	GM                                                      *
# Description:	Make file for building the game maker executable        *
#									*
#************************************************************************

!include "makers.mak"

install.exe:    install.obj\
                oldmouse.obj\
                windio.obj\
                scrnrout.obj\
                repprntf.obj\
                fullpath.obj
                $(LINKER) $(LFLAGS) @install.lnk

install.obj:    install.cpp
                $(CCOMPILER) $(CFLAGS) install.cpp
fullpath.obj:   fullpath.c
                $(CCOMPILER) $(CFLAGS) fullpath.c
windio.obj:     windio.c
                $(CCOMPILER) $(CFLAGS) windio.c
repprntf.obj:   repprntf.asm
                $(ASSEMBLER) $(AFLAGS) repprntf.asm
scrnrout.obj:   scrnrout.asm
                $(ASSEMBLER) $(AFLAGS) scrnrout.asm
