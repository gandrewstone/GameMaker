
        .MODEL LARGE
        LARGESMALL EQU 6
        IDEAL
        PUBLIC _drawblk
        PUBLIC _getblk

        EXTRN  _zeroaddon:WORD

        CODESEG

PROC    _drawblk
        SX      EQU [bp+LARGESMALL]
        SY      EQU [bp+2+LARGESMALL]
        BLOC    EQU [bp+4+LARGESMALL]

        push    bp
        mov     bp,sp
        push    ax
        push    bx
        push    cx
        push    dx
        push    di
        push    si
        push    ds
        push    es

        mov     ax,0A000h
        mov     es,ax

        mov     dx,320
        mov     ax,SY   ;map starting screen point
        mul     dx
        mov     di,ax
        add     di,SX
        add     di,[_zeroaddon]

        lds     si,BLOC
      
        mov     cx,20  ; width of box
        @@drawhoriz:
        push    cx
        mov     cx,10  ; length of box
        rep     movsw
        add     di,300
        pop     cx
      loop @@drawhoriz

@@endproc:

        pop     es
        pop     ds
        pop     si
        pop     di
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        pop     bp
        ret
ENDP    _drawblk

PROC    _getblk
        SX      EQU [bp+LARGESMALL]
        SY      EQU [bp+2+LARGESMALL]
        BLOC    EQU [bp+4+LARGESMALL]

        push    bp
        mov     bp,sp
        push    ax
        push    bx
        push    cx
        push    dx
        push    di
        push    si
        push    ds
        push    es
             
        mov     ax,0A000h
        mov     es,ax


        mov     ax,SY   ;map starting point into di
        mov     dx,320
        mul     dx
        mov     di,ax
        add     di,SX
        add     di,[_zeroaddon]

        lds     si, BLOC
        xchg    si,di
        mov     ax,ds
        push    es
        pop     ds
        mov     es,ax
        mov     cx,20  ; width of box
        @@drawhoriz:
        push    cx
        mov     cx, 20  ; length of box
        rep     movsb
        add     si,300
        pop     cx
      loop @@drawhoriz

@@endproc:
        pop     es
        pop     ds
        pop     si
        pop     di
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        pop     bp
        ret
ENDP    _getblk

END
