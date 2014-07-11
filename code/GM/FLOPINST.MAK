#************************************************************************
#									*
# File: 	GM                                                      *
# Description:	Make file for building the game maker executable        *
#									*
#************************************************************************

!include "makers.mak"

flopinst.exe:   flopinst.obj\
                scrnrout.obj\
                repprntf.obj\
                fullpath.obj\
                findfile.obj
                $(LINKER) $(LFLAGS) @flopinst.lnk


flopinst.obj:   flopinst.cpp
                $(CCOMPILER) $(CFLAGS) flopinst.cpp
fullpath.obj:   fullpath.c
                $(CCOMPILER) $(CFLAGS) fullpath.c
windio.obj:     windio.c
                $(CCOMPILER) $(CFLAGS) windio.c
findfile.obj:   findfile.cpp
                $(CCOMPILER) $(CFLAGS) findfile.cpp
repprntf.obj:   repprntf.asm
                $(ASSEMBLER) $(AFLAGS) repprntf.asm
scrnrout.obj:   scrnrout.asm
                $(ASSEMBLER) $(AFLAGS) scrnrout.asm
