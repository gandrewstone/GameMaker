; genbufa.asm - gena.asm, - unused routines and
; except writes to a memory buffer (Scrn).
; gena.asm   - generic 320x200x256 color routines
; Programmer:  Andy Stone    Created: 12/27/90
; Last Edited: 2/20/91

        .MODEL LARGE
        LARGESMALL EQU 6
        IDEAL
        P286
;        EXTRN  _zeroaddon:WORD                         Automatically in Scrn
        EXTRN  _xor:WORD
        PUBLIC _Point
        PUBLIC _FourPoint
        PUBLIC _GetCol
        PUBLIC _BoxFill
        PUBLIC _Gwritestr
        PUBLIC _PutBufToScrn
        PUBLIC _Scrn
        PUBLIC _drawblk
        PUBLIC _getblk

        CODESEG

_Scrn    dd     0

PROC    _drawblk
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
  

        les     di,[cs:_Scrn]

        mov     dx,320
        mov     ax,SY   ;map starting screen point
        mul     dx
        add     di,ax
        add     di,SX

        lds     si,BLOC
      
        mov     al,20  ; width of box
@@drawhoriz:

        mov     cx,10  ; length of box
        rep     movsw
        add     di,300
        sub     al,1
        jnz     @@drawhoriz

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
ENDP    _drawblk

PROC    _getblk
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
             
        les     di,[cs:_Scrn]

        mov     ax,SY   ;map starting point into di
        mov     dx,320
        mul     dx
        add     di,ax
        add     di,SX

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
;        pop     dx
;        pop     cx
;        pop     bx
;        pop     ax
        pop     bp
        ret
ENDP    _getblk


PROC    _PutBufToScrn
        push     si
        push     di
        push     es
        push     ds

        lds      si,[cs:_Scrn]

        mov      ax,0A000h
        mov      es,ax
        mov      di,0

        mov      cx,65535/2
        rep      movsw

        pop      ds
        pop      es
        pop      di
        pop      si
        ret
ENDP    _PutBufToScrn


PROC    _BoxFill

        SX      EQU [bp]
        SY      EQU [bp+2]
        SX1     EQU [bp+4]
        SY1     EQU [bp+6]
        COL     EQU [bp+8]

        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
        push    ax
        push    bx
        push    cx
        push    dx
        push    di
        push    es
        
        les     di,[cs:_Scrn]

        mov     ax,SY   ;map starting point into di
        mov     dx,320
        mul     dx
        add     di,ax
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


PROC    _Point
        COL     EQU    [bp+4]
        SY      EQU    [bp+2]
        SX      EQU    [bp]

        push    bp
        mov     bp,sp
        add     bp,LARGESMALL         ; 6 to turn this into a far procedure
        push    es
        push    di
        push    ax
        push    dx

        les     di,[cs:_Scrn]

        mov     ax,SY                ; Find memory spot
        mov     dx,320
        mul     dx
        add     di,ax
        add     di,SX

        cmp     [_xor],0             ; xor or not?
        je      @@noxor              ; if xor is 0 overwrite color
        mov     al,[BYTE es:di]      ; xor col with current color
        xor     ax,COL
        jmp     @@dostore
@@noxor:
        mov     ax,COL

@@dostore:
        stosb

        
        pop     dx
        pop     ax
        pop     di
        pop     es
        pop     bp 
        ret
ENDP    _Point

PROC    _FourPoint
        SY      EQU    [bp+2]
        SX      EQU    [bp]

        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
        push    es
        push    di
        push    ds
        push    si
        push    ax
        push    dx

        les     di,[cs:_Scrn]

        mov     ax,SY                ; Find memory spot
        mov     dx,320
        mul     dx
        add     di,ax
        add     di,SX

        lds     si,[bp+4]
        mov     cx,4
        rep movsb
        
        pop     dx
        pop     ax
        pop     si
        pop     ds
        pop     di
        pop     es
        pop     bp 
        ret
ENDP    _FourPoint

PROC    _GetCol

        SY      EQU    [bp+2]
        SX      EQU    [bp]

        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
        push    ds
        push    si
        push    dx

        
        lds     si,[cs:_Scrn]
        add     si,SX
         
        mov     ax,SY
        mov     dx,320
        mul     dx
        add     si,ax
        mov     ax,0
        lodsb

        pop     dx
        pop     si
        pop     ds
        pop     bp
        ret
ENDP    _GetCol

; WRITES DIRECTLY TO THE SCREEN STILL
PROC    _Gwritestr
        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
        pusha
        mov     ah,13h
        mov     al,0h
        mov     bh,0h
        mov     bl,[BYTE bp+4]
        mov     cx,[WORD bp+10]
        mov     dh,[BYTE bp+2]
        mov     dl,[BYTE bp]
        les     bp,[bp+6]
        int     10h
        popa 
        pop     bp
        ret
ENDP _Gwritestr        
END
