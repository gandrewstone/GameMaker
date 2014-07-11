#************************************************************************
#									*
# File: 	GM                                                      *
# Description:	Make file for building the game maker executable        *
#									*
#************************************************************************

!include "makers.mak"

menu.exe:       menu.obj\
                startupl.obj\
                oldmouse.obj\
                gma.obj gmgen.h\
                windio.obj\
                findfile.obj\
                genc.obj\
                scrnrout.obj
                $(LINKER) $(LFLAGS) @menu.lnk
!include "compile.mak"

