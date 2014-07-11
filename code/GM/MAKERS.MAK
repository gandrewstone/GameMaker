CCOMPILER=bcc
ASSEMBLER=tasm
LINKER=tlink

# NORMAL
!if $(OPTION)==0
CFLAGS=-f- -c -C -ml -1 -P -O2 -Ic:\drv
AFLAGS=/ml/jP286N/m2
LFLAGS=/s/c
!endif

# DEBUG
!if $(OPTION)==1
CFLAGS=-f- -c -C -ml -v -1 -P -N -O2
AFLAGS=/zi/ml/m2/jP286N
LFLAGS=/m/s/c/v
!endif

# PROFILE
!if $(OPTION)==2
CFLAGS= -K2 -c -ml -Vo -P -v
AFLAGS=/zi/ml/m2/jP386
LFLAGS=/m/s/c/v
LINKER=c:\lang\profile\tlink
!endif



