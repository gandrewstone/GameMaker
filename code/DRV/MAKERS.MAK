CCOMPILER=bcc
ASSEMBLER=tasmx
LINKER=tlink
LIBRARIAN=tlib

!if $(OPTION)==0
CFLAGS=-c -C -ml -1 -P -O2
AFLAGS=/ml/jP386N/m2
LFLAGS=/s/c/3
LIBFLAGS=/C
!endif

!if $(OPTION)==1
CFLAGS=-f -c -C -ml -v -1 -P -N -O2
AFLAGS=/zi/ml/m2/jP386N
LFLAGS=/m/s/c/v/3
LIBFLAGS=/C
!endif



