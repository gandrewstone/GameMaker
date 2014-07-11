# Make include file for the new mouse routines.
#newmouse.obj:   newmouse.asm
#                $(ASSEMBLER) $(AFLAGS) newmouse.asm
geninput.obj:   geninput.cpp
                $(CCOMPILER) $(CFLAGS) geninput.cpp
genclass.obj:   genclass.cpp
                $(CCOMPILER) $(CFLAGS) genclass.cpp
timer.obj:      timer.cpp
                $(CCOMPILER) $(CFLAGS) timer.cpp
gasclass.obj:   gasclass.cpp
                $(CCOMPILER) $(CFLAGS) gasclass.cpp
windclss.obj:   windclss.cpp
                $(CCOMPILER) $(CFLAGS) windclss.cpp


