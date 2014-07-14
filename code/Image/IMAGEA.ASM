

        IDEAL
        MODEL LARGE
        CALLSIZE EQU 6

PROC   GetBits   far
       This EQU bp+LARGESMALL
       mov      bp,sp
       push     ds
       push     si

       lds,si   [This]


       mov      ax,

       pop      si
       pop      ds
       pop      bp
       ret
ENDP   GetBits
