#************************************************************************
#									*
# File: 	IMAGE                                                   *
# Description:	Make file for building the GIF reader executable        *
#									*
#************************************************************************

!include "makers.mak"

image.exe:      image.obj\
                startupl.obj\
                setenvp.obj\
                setargv.obj\
                gifc.obj\
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
                pala.obj\
                gena.obj\
                jsticka.obj\
                facelift.obj\
                graphc.obj\
                scrnrout.obj
                $(LINKER) $(LFLAGS) @image.lnk

!include "compile.mak"
