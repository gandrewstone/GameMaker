!include "newmouse.mak"
!include "..\depend.mak"

startupl.obj:   startup.asm
                $(ASSEMBLER) $(AFLAGS) /M /MX /Q /T /D__LARGE__ startup.asm, startupl.obj
setenvp.obj:    setenvp.asm
                $(ASSEMBLER) $(AFLAGS) /M /MX /Q /T /D__LARGE__ setenvp.asm, setenvp.obj
setargv.obj:    setargv.asm
                $(ASSEMBLER) $(AFLAGS) /M /MX /Q /T /D__LARGE__ setargv.asm, setargv.obj

palchos.obj:    palchos.c\
                palette.h\
                tranmous.hpp\
                gasclass.hpp\
                genclass.hpp\
                geninput.hpp\
                timer.hpp\
                windclss.hpp
                $(CCOMPILER) $(CFLAGS) palchos.c
charedit.obj:   charedit.c\
                tranmous.hpp
                $(CCOMPILER) $(CFLAGS) charedit.c
mapmaker.obj:   mapmaker.c\
                tranmous.hpp
                $(CCOMPILER) $(CFLAGS) mapmaker.c
grator.obj:     grator.c grator.h
                $(CCOMPILER) $(CFLAGS) grator.c
blocedit.obj:   blocedit.c\
                tranmous.hpp
                $(CCOMPILER) $(CFLAGS) blocedit.c
image.obj:      image.c\
                tranmous.hpp
                $(CCOMPILER) $(CFLAGS) image.c
monedit.obj:    monedit.c
                $(CCOMPILER) $(CFLAGS) monedit.c
utility.obj:    utility.c gmgen.h
                $(CCOMPILER) $(CFLAGS) utility.c
gifc.obj:       gifc.c
                $(CCOMPILER) $(CFLAGS) gifc.c
gramap.obj:     gramap.c grator.h
                $(CCOMPILER) $(CFLAGS) gramap.c
soundc.obj:     soundc.c
                $(CCOMPILER) $(CFLAGS) soundc.c
mapc.obj:       mapc.c
                $(CCOMPILER) $(CFLAGS) mapc.c
windio.obj:     windio.c
                $(CCOMPILER) $(CFLAGS) windio.c
genc.obj:       mousefn.h genc.c
                $(CCOMPILER) $(CFLAGS) genc.c
vgaobj.obj:     c:\drv\vgaobj.asm
                $(ASSEMBLER) $(AFLAGS) c:\drv\vgaobj.asm
fileclss.obj:   c:\drv\fileclss.cpp c:\drv\fileclss.hpp
                $(CCOMPILER) $(CFLAGS) c:\drv\fileclss.cpp
fli.obj:        c:\image\fli\fli.cpp c:\drv\fileclss.hpp
                $(CCOMPILER) $(CFLAGS) c:\image\fli\fli.cpp
palc.obj:       palc.c
                $(CCOMPILER) $(CFLAGS) palc.c
blocc.obj:      blocc.c
                $(CCOMPILER) $(CFLAGS) blocc.c
bloca.obj:      bloca.asm
                $(ASSEMBLER) $(AFLAGS) bloca.asm
clrbloca.obj:   clrbloca.asm
                $(ASSEMBLER) $(AFLAGS) clrbloca.asm
pala.obj:       pala.asm
                $(ASSEMBLER) $(AFLAGS) pala.asm
gena.obj:       gena.asm
                $(ASSEMBLER) $(AFLAGS) gena.asm
jsticka.obj:    jsticka.asm
                $(ASSEMBLER) $(AFLAGS) jsticka.asm
oldmouse.obj:   oldmouse.asm
                $(ASSEMBLER) $(AFLAGS) oldmouse.asm
jstickc.obj:    jstickc.c
                $(CCOMPILER) $(CFLAGS) jstickc.c
facelift.obj:   facelift.c
                $(CCOMPILER) $(CFLAGS) facelift.c
graphc.obj:     graphc.c
                $(CCOMPILER) $(CFLAGS) graphc.c
findfile.obj:   findfile.cpp
                $(CCOMPILER) $(CFLAGS) findfile.cpp

