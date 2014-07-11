
; gena.asm   - generic 320x200x256 color routines
; Programmer:  Andy Stone    Created: 12/27/90
; Last Edited: 2/20/91

        IDEAL
        P286
        MODEL LARGE
        LARGESMALL EQU 6
        EXTRN  _zeroaddon:WORD
        EXTRN  _xor:WORD
        PUBLIC _Point
        PUBLIC _GetCol
        PUBLIC _BoxFill
        PUBLIC _Gwritestr
        PUBLIC _GetROMFont

        CODESEG

PROC    _GetROMFont
        push       es
        push       bp
        mov        ax,1130h
        mov        bh,3
        int        10h

        mov        dx,es
        mov        ax,bp
        pop        bp
        pop        es
        ret
ENDP    _GetROMFont

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

        mov     ax,0A000h
        mov     es,ax

        mov     ax,SY                ; Find memory spot
        mov     dx,320
        mul     dx
        mov     di,ax
        add     di,SX
        add     di,[_zeroaddon]

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

PROC    _GetCol

        SY      EQU    [bp+2]
        SX      EQU    [bp]

        push    bp
        mov     bp,sp
        add     bp,LARGESMALL
        push    ds
        push    si
        push    dx

        
        mov     si,[_zeroaddon]   ; Put into bx, 'cause DS is changed
        add     si,SX
         
        mov     ax,0A000h
        mov     ds,ax

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