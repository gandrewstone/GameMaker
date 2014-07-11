; gma.asm Routines needed by GM (save space).
; Programmer: Andy Stone    Created: Mar 4, 94
; Last Edited: Mar 4, 1994

        IDEAL
        MODEL LARGE
        LARGESMALL EQU 6

        PUBLIC _Palette

        CODESEG

PROC    _Palette

        push    bp
        mov     bp,sp

        mov     dx,03C8h           ; VGA color number port
        mov     al,[bp+LARGESMALL] ; color number
        out     dx,al

        inc     dx                 ; RBG color additive port

        mov     al,[bp+2+LARGESMALL]          ; user's red
        out     dx,al
        mov     al,[bp+4+LARGESMALL]          ; user's green
        out     dx,al
        mov     al,[bp+6+LARGESMALL]          ; user's blue
        out     dx,al

        pop     bp
        ret
ENDP    _Palette

END

