PROC    _BoxFill

        SX      EQU [bp+CALLSIZE]
        SY      EQU [bp+2+CALLSIZE]
        SX1     EQU [bp+4+CALLSIZE]
        SY1     EQU [bp+6+CALLSIZE]
        COL     EQU [bp+8+CALLSIZE]

        push    bp
        mov     bp,sp
        push    ax
        push    bx
        push    cx
        push    dx
        push    di
        push    es
        
        mov     ax,0A000h
        mov     es,ax

        mov     ax,SY   ;map starting point into di
        mov     dx,320
        mul     dx
        mov     di,ax
        add     di,SX

        mov     dx,SX1  ; Calculate length of box
        sub     dx,SX
        inc     dx

        mov     bx,320  ; Calculate amount to add to get to the next line
        sub     bx,dx

        mov     cx,SY1  ; Calculate width of box
        sub     cx,SY
        inc     cx
        
        mov     al,COL     
@@drawhoriz:
        push    cx
        mov     cx,dx
        rep     stosb
        add     di,bx
        pop     cx
      loop @@drawhoriz
        pop     es
        pop     di
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        pop     bp
        ret
ENDP    _BoxFill


