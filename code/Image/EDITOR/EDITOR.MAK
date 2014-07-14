!include "d:\makers.mak"
!include "d:\depend.mak"

editor.exe:   editor.obj\
              Image.obj\
              fileclss.obj\
              fli.obj\
              findfile.obj\
              textio.obj fontchr.obj bitmap.obj\
              $(VidDrvLIBs)\
              $(InputOBJs)
              $(LINKER) $(LFLAGS) @editor.lnk

editor.obj:   editor.cpp\
              $(VidDrvHEADERs)\
              $(FontHEADERs)\
              $(ImageHEADERs)\
              $(InputHEADERs)
              $(CCOMPILER) $(CFLAGS) editor.cpp

fli.obj:      d:\image\fli\fli.cpp\
              $(VidDrvHEADERs)
              $(CCOMPILER) $(CFLAGS) d:\image\fli\fli.cpp

fileclss.obj: d:\drv\fileclss.hpp d:\drv\fileclss.cpp
              $(CCOMPILER) $(CFLAGS) d:\drv\fileclss.cpp


textio.obj:   d:\font\textio.cpp\
              d:\drv\fileclss.hpp\
              $(FontHEADERs)\
              $(VidDrvHEADERs)
              $(CCOMPILER) $(CFLAGS) d:\font\textio.cpp

fontchr.obj:  d:\font\fontchr.cpp\
              $(FontHEADERs)\
              $(VidDrvHEADERs)
              $(CCOMPILER) $(CFLAGS) d:\font\fontchr.cpp

bitmap.obj:   d:\drv\bitmap.cpp\
              $(VidDrvHEADERs)
              $(CCOMPILER) $(CFLAGS) d:\drv\bitmap.cpp

findfile.obj: findfile.cpp
              $(CCOMPILER) $(CFLAGS) findfile.cpp

!include "geninput.mak"
!include "d:\image\image.mak"

