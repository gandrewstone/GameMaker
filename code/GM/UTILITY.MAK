#************************************************************************
#									*
# File: 	GM                                                      *
# Description:	Make file for building the game maker executable        *
#									*
#************************************************************************

!include "makers.mak"

utility.exe:    utility.obj\
                jsticka.obj\
                jstickc.obj\
                startupl.obj\
                oldmouse.obj\
                windio.obj\
                genc.obj\
                gma.obj\
                findfile.obj\
                scrnrout.obj
                $(LINKER) $(LFLAGS) @utility.lnk

#include "compile.mak"
