#************************************************************************
#									*
# File: 	GRATOR                                                  *
# Description:	Make file for building the integrator executable        *
#									*
#************************************************************************

!include "makers.mak"

grator.exe:     grator.obj\
                gramap.obj\
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
                findfile.obj\
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
                $(LINKER) $(LFLAGS) @grator.lnk
!include "compile.mak"
