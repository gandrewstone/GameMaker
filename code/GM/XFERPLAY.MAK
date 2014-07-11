#************************************************************************
#									*
# File: 	PLAYGAME                                                *
# Description:	Make file for building the playgame executable          *
#									*
#************************************************************************

!include "makers.mak"

xferplay.exe:   startupl.obj\
                xferplay.obj\
                regspace.obj\
                oldmouse.obj\
                soundc.obj\
                windio.obj\
                gencxfer.obj\
                gifc.obj\
                palc.obj\
                svgac.obj\
                hiscorel.obj\
                jsticka.obj\
                jstickc.obj\
                pala.obj\
                scrnrout.obj\
                sbmusic.obj\
                dirtrect.obj\
                membloca.obj\
                graphc.obj\
                bloca.obj\
                microcnl.obj\
                svgaa.obj
                $(LINKER) $(LFLAGS) @xferplay.lnk

xferplay.obj:   playgame.c\
                gmgen.h mousefn.h
                $(CCOMPILER) $(CFLAGS) -DTRANSFER=1 -oxferplay.obj playgame.c

gencxfer.obj:   mousefn.h gmgen.h genc.c
                $(CCOMPILER) $(CFLAGS) -DTRANSFER=1 -ogencxfer.obj genc.c

!include "compile.mak"
