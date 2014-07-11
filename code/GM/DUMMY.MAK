#************************************************************************
#									*
# File:   DUMMY                                            *
# Description:	Make file for building the palette editor executable    *
#									*
#************************************************************************

!include "makers.mak"

dummy.exe:      dummy.obj\
                setenvp.obj\
                setargv.obj\
                startupl.obj
                $(LINKER) $(LFLAGS) @dummy.lnk
dummy.obj:      dummy.c
                $(CCOMPILER) $(CFLAGS) dummy.c


!include "compile.mak"
