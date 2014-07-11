#************************************************************************
#									*
# File: 	SNDEDIT                                                 *
# Description:	Make file for building the sound editor executable      *
#									*
#************************************************************************

!include "makers.mak"

sndedit.exe:    sndedit.obj\
                startupl.obj\
                newmouse.obj\
                genclass.obj\
                gasclass.obj\
                geninput.obj\
                timer.obj\
                windclss.obj\
                soundc.obj\
                windio.obj\
                genc.obj\
                palc.obj\
                pala.obj\
                gena.obj\
                jstickc.obj\
                facelift.obj\
                jsticka.obj\
                graphc.obj\
                scrnrout.obj
                $(LINKER) $(LFLAGS) @sndedit.lnk

!include "compile.mak"
