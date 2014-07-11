#************************************************************************
#									*
# File: 	CHAREDIT                                                 *
# Description:	Make file for building the character editor executable  *
#									*
#************************************************************************

!include "makers.mak"

charedit.exe:   charedit.obj\
                startupl.obj\
                setenvp.obj\
                setargv.obj\
                newmouse.obj\
                soundc.obj\
                windio.obj\
                genc.obj\
                palc.obj\
                blocc.obj\
                bloca.obj\
                pala.obj\
                gena.obj\
                jsticka.obj\
                jstickc.obj\
                facelift.obj\
                graphc.obj\
                scrnrout.obj
                $(LINKER) $(LFLAGS) @charedit.lnk
#include "compile.mak"
