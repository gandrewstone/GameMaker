#************************************************************************
#									*
# File: 	BLOCEDIT                                                *
# Description:	Make file for building the block edit executable        *
#									*
#************************************************************************

!include "makers.mak"

blocedit.exe:   blocedit.obj\
                startupl.obj\
                setenvp.obj\
                setargv.obj\
                findfile.obj\
                newmouse.obj\
                genclass.obj\
                gasclass.obj\
                geninput.obj\
                timer.obj\
                windclss.obj\
                windio.obj\
                genc.obj\
                blocc.obj\
                palc.obj\
                gena.obj\
                bloca.obj\
                pala.obj\
                jsticka.obj\
                facelift.obj\
                genclass.obj\
                gasclass.obj\
                geninput.obj\
                timer.obj\
                windclss.obj\
                graphc.obj\
                scrnrout.obj
                $(LINKER) $(LFLAGS) @blocedit.lnk

!include "compile.mak"

