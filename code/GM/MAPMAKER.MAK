#************************************************************************
#									*
# File: 	MAPMAKER                                                *
# Description:	Make file for building the map editor executable        *
#									*
#************************************************************************

!include "makers.mak"

mapmaker.exe:   mapmaker.obj\
                startupl.obj\
                setenvp.obj\
                setargv.obj\
                mapc.obj\
                newmouse.obj\
                genclass.obj\
                gasclass.obj\
                geninput.obj\
                timer.obj\
                windclss.obj\
                windio.obj\
                genc.obj\
                palc.obj\
                blocc.obj\
                bloca.obj\
                clrbloca.obj\
                pala.obj\
                gena.obj\
                jsticka.obj\
                facelift.obj\
                graphc.obj\
                scrnrout.obj
                $(LINKER) $(LFLAGS) @mapmaker.lnk

!include "compile.mak"
