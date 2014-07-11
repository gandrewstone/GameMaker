#************************************************************************
#									*
# File: 	IMAGE                                                   *
# Description:	Make file for building the GIF reader executable        *
#									*
#************************************************************************

!include "makers.mak"

disp.exe:       makers.mak\
                disp.obj\
                startupl.obj\
                newgifc.obj\
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
                $(LINKER) $(LFLAGS) @disp.lnk

newgifc.obj:    newgifc.c
                $(CCOMPILER) $(CFLAGS) newgifc.c

!include "compile.mak"
