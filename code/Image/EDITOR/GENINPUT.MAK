# Make include file for the new mouse routines.
intsa.obj:      d:\input\intsa.asm
                $(ASSEMBLER) $(AFLAGS) d:\input\intsa.asm
geninput.obj:   d:\input\geninput.cpp
                $(CCOMPILER) $(CFLAGS) d:\input\geninput.cpp
genclass.obj:   d:\input\genclass.cpp
                $(CCOMPILER) $(CFLAGS) d:\input\genclass.cpp
timer.obj:      d:\input\timer.cpp
                $(CCOMPILER) $(CFLAGS) d:\input\timer.cpp
gasclass.obj:   d:\input\gasclass.cpp
                $(CCOMPILER) $(CFLAGS) d:\input\gasclass.cpp
jsticka.obj:    d:\input\jsticka.asm
                $(ASSEMBLER) $(AFLAGS) d:\input\jsticka.asm
jstickc.obj:    d:\input\jstickc.c
                $(CCOMPILER) $(CFLAGS) d:\input\jstickc.c

