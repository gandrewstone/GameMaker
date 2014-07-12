; Copyright 1994 G. Andrew Stone                                05/26/94
; NOT PUBLIC DOMAIN!
;------------

MACRO   pushall
        push    bp
        mov     bp,sp

        pusha
        push    es
        push    ds
ENDM

MACRO   popall
        pop    ds
        pop    es
        popa
        pop    bp
ENDM

MACRO   pushr registers
        IRP   reg,<registers>
          push  reg
        ENDM
ENDM

MACRO   popr registers
        IRP   reg,<registers>
          pop  reg
        ENDM
ENDM


