!include "d:\makers.mak"
!include "d:\depend.mak"

ImageHEADERs = d:\image\image.h $(VidDrvHEADERs)

image.obj:   d:\image\image.cpp $(ImageHEADERs)
             $(CCOMPILER) $(CFLAGS) d:\image\image.cpp


