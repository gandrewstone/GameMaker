#************************************************************************
#									*
# File: 	PALCHOS                                                 *
# Description:	Make file for building the palette editor executable    *
#									*
#************************************************************************

!include "makers.mak"

palchos.exe:    palchos.obj\
                startupl.obj\
                setenvp.obj\
                setargv.obj\
                genc.obj\
                palc.obj\
                pala.obj\
                newmouse.obj\
                jsticka.obj\
                gena.obj\
                windio.obj\
                facelift.obj\
                graphc.obj\
                genclass.obj\
                gasclass.obj\
                geninput.obj\
                timer.obj\
                windclss.obj\
                findfile.obj\
                scrnrout.obj
                $(LINKER) $(LFLAGS) @palchos.lnk

!include "compile.mak"
