#************************************************************************
#									*
# File: 	PLAYGAME                                                *
# Description:	Make file for building the playgame executable          *
#									*
#************************************************************************

!include "makers.mak"

playgame.exe:   playgame.obj\
                regspace.obj\
                oldmouse.obj\
                findfile.obj\
                soundc.obj\
                windio.obj\
                genc.obj\
                gifc.obj\
                palc.obj\
                svgac.obj\
                vgaobj.obj\
                fli.obj\
                hiscorel.obj\
                jsticka.obj\
                jstickc.obj\
                pala.obj\
                scrnrout.obj\
                sbmusic.obj\
                dirtrect.obj\
                membloca.obj\
                fileclss.obj\
                graphc.obj\
                bloca.obj\
                microcnl.obj\
                svgaa.obj
                $(LINKER) $(LFLAGS) @playgame.lnk

playgame.obj:   playgame.c gmgen.h mousefn.h
                $(CCOMPILER) $(CFLAGS) -I..\Image\fli playgame.c

!include "compile.mak"

