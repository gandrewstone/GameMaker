
        IDEAL
        MODEL LARGE
        P386N
        LARGESMALL EQU 6

        PUBLIC _TextMode
        PUBLIC _Palette
        PUBLIC _GetAllPal
        PUBLIC _SetAllPal
        PUBLIC _SetAllPalTo

        CODESEG

SAVEREGS       =      0
KILLREGS       =      1
;SaveKillRegs

RETRACE_PORT   EQU      03DAh
VERT           EQU      8
HORIZ          EQU      1
ON             =       1
OFF            =       0

MACRO   WaitRetrace HorizVert,OnOff, SaveR
LOCAL   Top
        IFB     <SaveR>
        push    ax
        push    dx
        ELSEIF  SaveR EQ SAVEREGS
        push    ax
        push    dx
        ENDIF
Top:
        mov     dx,RETRACE_PORT
        in      al,dx
        test    al,HorizVert
        IF      OnOff EQ ON
        jnz     Top
        ELSE
        jz      Top
        ENDIF
        IFB     <SaveR>
        pop     dx
        pop     ax
        ELSEIF  SaveR EQ SAVEREGS
        pop     dx
        pop     ax
        ENDIF
ENDM


MACRO   PUSHCKILLS
        push      ax
        push      bx
        push      cx
        push      dx
ENDM

MACRO   POPCKILLS
        pop       dx
        pop       cx
        pop       bx
        pop       ax
ENDM


PROC    _SetAllPalTo FAR
        PALADDR   EQU [bp+LARGESMALL]
        push    bp
        mov     bp,sp
        push    ds
        push    si
        lds     si,PALADDR
        mov     al,0
        mov     dx,03c8h
        mov     bx,256

        out     dx,al
        inc     dx
@@top:
        mov     cx,3
        rep     outsb
        sub     si,3
        dec     bx
        cmp     bx,0
        jg      @@top

        pop     si
        pop     ds
        pop     bp
        ret
ENDP    _SetAllPalTo


PROC    _SetAllPal FAR
        PALADDR   EQU [bp+LARGESMALL]
        push    bp
        mov     bp,sp
        push    ds
        push    si

        lds     si,PALADDR
        mov     al,0
        mov     dx,03c8h

        out     dx,al
        inc     dx
        mov     cx,768
        rep     outsb
        pop     si
        pop     ds
        pop     bp
        ret
ENDP    _SetAllPal

PROC    _GetAllPal
        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
        push    ax
        push    bx
        push    cx
        push    dx
        push    es

        mov     ah,10h
        mov     al,17h
        mov     bx,0
        mov     cx,256
        les     dx,[bp]
        int     10h

        pop     es
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        pop     bp
        ret
ENDP    _GetAllPal


;PROC    _GetAllPal FAR
;        PALADDR   EQU [bp+LARGESMALL]
;        push    bp
;        mov     bp,sp
;        push    es
;        push    di
;
;        les     di,PALADDR
;
;        mov     al,0
;        mov     dx,03c8h
;
;        out     dx,al
;        inc     dx
;        mov     cx,768
;        rep     insb
;
;        pop     di
;        pop     es
;        pop     bp
;        ret
;ENDP    _GetAllPal

PROC    _Palette FAR

        push    bp
        mov     bp,sp

        mov     dx,03C8h           ; VGA color number port
        mov     al,[bp+LARGESMALL] ; color number
        out     dx,al

        inc     dx                 ; RBG color additive port

        mov     al,[bp+2+LARGESMALL]          ; user's red
        out     dx,al
        mov     al,[bp+3+LARGESMALL]          ; user's green
        out     dx,al
        mov     al,[bp+4+LARGESMALL]          ; user's blue
        out     dx,al

        pop     bp
        ret
ENDP    _Palette

PROC    _TextMode   FAR
        mov      ax,03h
        int      10h
        retf
ENDP    _TextMode

END


