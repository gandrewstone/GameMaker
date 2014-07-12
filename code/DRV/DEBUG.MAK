#************************************************************************
#									*
#									*
#************************************************************************

!include "makers.mak"

debug.exe:      debug.obj stlth64.obj
                $(LINKER) $(LFLAGS) @debug.lnk

debug.obj:      debug.cpp
                $(CCOMPILER) $(CFLAGS) debug.cpp

