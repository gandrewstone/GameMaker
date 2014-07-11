; clrbloca.asm - only mapmaker uses drawcbloc, so I'm putting it here to save
; space.  Andy.

        .MODEL LARGE
        LARGESMALL EQU 6
        IDEAL

        PUBLIC _drawcbloc

        EXTRN  _zeroaddon:WORD
        

        CODESEG

PROC    _drawcbloc
        SX      EQU [bp]
        SY      EQU [bp+2]
        BLOC    EQU [bp+4]

        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
;        push    ax
;        push    bx
;        push    cx
;        push    dx
        push    di
        push    si
        push    ds
        push    es
             
        mov     ax,0A000h
        mov     es,ax

      
        mov     ax,SY     ;map starting point into di
        mov     dx,320
        mul     dx
        mov     di,ax
        add     di,SX
        add     di,[_zeroaddon]

        lds     si, BLOC  ;Map ds:si as the block address

        mov     cx,20  ; width of box
@@drawhoriz:
        push    cx
        mov     cx,20  ; length of box
@@drawpt:
        lodsb
        cmp     al,255
        je      @@skip        
        stosb
      loop @@drawpt
      jmp @@continue
@@skip:
        inc     di
      loop @@drawpt
@@continue:
        add     di,300
        pop     cx
      loop @@drawhoriz

@@endproc:
        pop     es
        pop     ds
        pop     si
        pop     di
;        pop     dx
;        pop     cx
;        pop     bx
;        pop     ax
        pop     bp
        ret
ENDP    _drawcbloc
END
