#************************************************************************
#									*
# File: 	MONEDIT                                                 *
# Description:	Make file for building the monster editor executable    *
#									*
#************************************************************************

!include "makers.mak"

monedit.exe:    monedit.obj\
                startupl.obj\
                setenvp.obj\
                setargv.obj\
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
                $(LINKER) $(LFLAGS) @monedit.lnk

!include "compile.mak"
